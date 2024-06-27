#include <doctest/doctest.h>

#include "../zq_testing.hpp"
#include <chrono>
#include <functional>


struct Poll {

  using SocketRef = std::reference_wrapper<zq::Socket>;
  using Callback = std::function<void(zq::Socket&)>;
  using Actor = std::pair<SocketRef, Callback>;
  using Actors = std::vector<Actor>;

  explicit Poll(Actors actors) : entries(std::move(actors)) {
    poll_items.reserve(entries.size());
    for (auto& [socket, _] : entries) {
      poll_items.push_back({socket.get().socket_ptr.get(), 0, ZMQ_POLLIN, 0});
    }
  }

  int await(std::chrono::milliseconds timeout) {
    long t_o = static_cast<long>(timeout.count());
    int num_items = static_cast<int>(poll_items.size());
    auto retval = zmq_poll(poll_items.data(), num_items, t_o);
    for (size_t i = 0; i < poll_items.size(); ++i) {
      if (poll_items[i].revents & ZMQ_POLLIN) {
        entries[i].second(entries[i].first);
      }
    }
    return retval;
  }

private:
  Actors entries;
  std::vector<zmq_pollitem_t> poll_items;
};


SCENARIO("Multi Socket Polling") {
  auto context = zq::mk_context();
  REQUIRE(context);
  using namespace std::chrono_literals;

  GIVEN("a list of poll actors") {
    auto [client1, server1] = pp_cs_sockets(*context, next_inproc_address());
    auto [client2, server2] = pp_cs_sockets(*context, next_inproc_address());
    auto [client3, server3] = pp_cs_sockets(*context, next_inproc_address());

    WHEN("checking the size of the message") {

      Poll::Actors actors;
      bool server1_called = false;
      bool server2_called = false;
      bool server3_called = false;

      actors.push_back({server1, [&server1_called](zq::Socket& socket) {
        auto reply = socket.recv();
        REQUIRE(reply);
        REQUIRE(reply.value());
        auto restored = zq::restore_as<std::string>(*reply.value());
        REQUIRE_EQ(restored, "first");
        server1_called = true;
      }});
      actors.push_back({server2, [&server2_called](zq::Socket& socket) {
        auto reply = socket.recv();
        REQUIRE(reply);
        REQUIRE(reply.value());
        auto restored = zq::restore_as<std::string>(*reply.value());
        REQUIRE_EQ(restored, "second");
        server2_called = true;
      }});
      actors.push_back({server3, [&server3_called](zq::Socket& socket) {
        auto reply = socket.recv();
        REQUIRE(reply);
        REQUIRE(reply.value());
        auto restored = zq::restore_as<std::string>(*reply.value());
        REQUIRE_EQ(restored, "third");
        server3_called = true;
      }});


      Poll poll(std::move(actors));

      THEN("events on a socket can be awaited") {
        auto rc_1 = client1.send(zq::typed_message("first"));
        REQUIRE(rc_1);
        int num_events = poll.await(500ms);
        REQUIRE_EQ(num_events, 1);
        REQUIRE(server1_called);
      }

      AND_THEN("events on multiple socket can be awaited") {
        auto rc_1 = client1.send(zq::typed_message("first"));
        REQUIRE(rc_1);
        auto rc_2 = client2.send(zq::typed_message("second"));
        REQUIRE(rc_2);
        auto rc_3 = client3.send(zq::typed_message("third"));
        REQUIRE(rc_3);
        int num_events = poll.await(500ms);
        REQUIRE_EQ(num_events, 3);
        REQUIRE(server1_called);
        REQUIRE(server2_called);
        REQUIRE(server3_called);
      }

    }
  }
}
