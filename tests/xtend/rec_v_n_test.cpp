#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <zq/zq.hpp>

std::tuple<zq::Socket, zq::Socket> pp_cs_sockets(zq::Context& context,
                                                 std::string_view address) {
  auto server = context.bind(zq::SocketType::REP, address);
  auto client = context.connect(zq::SocketType::REQ, address);
  REQUIRE(client);
  REQUIRE(server);
  return {std::move(*client), std::move(*server)};
}

enum class RecvResult { Ok, Underflow, Overflow };

template <size_t N>
struct RecvData {
  size_t msg_count{0};
  std::array<zq::Message, N> messages{};

  RecvResult result() {
    if (msg_count < N) {
      return RecvResult::Underflow;
    }
    if (msg_count > N) {
      return RecvResult::Overflow;
    }
    return RecvResult::Ok;
  }
};

template <size_t N>
auto recv_n(zq::Socket& socket)
    -> std::optional<tl::expected<RecvData<N>, std::runtime_error>> {
  RecvData<N> data{};
  auto* socket_ptr = socket.socket_ptr.get();
  using zq::currentZmqRuntimeError;

  auto rc = zmq_msg_recv(std::addressof(data.messages[0].msg), socket_ptr,
                         ZMQ_DONTWAIT);

  if (rc == -1) {
    if (zmq_errno() == EAGAIN) {
      return std::nullopt;
    } else {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
  }
  data.msg_count++;

  int more = 0;
  size_t more_size = sizeof(more);
  // read expected num of messages, no more data, return what we have
  for (size_t i = 1; i < N; ++i) {
    rc = zmq_getsockopt(socket_ptr, ZMQ_RCVMORE, &more, &more_size);
    if (rc == -1) {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
    if (!more) {
      return data;
    }
    rc = zmq_msg_recv(std::addressof(data.messages[i].msg), socket_ptr, 0);
    if (rc == -1) {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
    data.msg_count++;
  }
  // one more 'more' check for the overflow case is required
  rc = zmq_getsockopt(socket_ptr, ZMQ_RCVMORE, &more, &more_size);
  if (rc == -1) {
    return tl::make_unexpected(currentZmqRuntimeError());
  }
  if (more) {
    data.msg_count++;
    ;
  }

  return data;
}

SCENARIO("Testing recv_n") {
  auto context = zq::mk_context();
  REQUIRE(context);

  GIVEN("a request, a reply socket") {
    std::string_view address = "ipc://localhost_5556";
    auto [client, server] = pp_cs_sockets(*context, address);

    WHEN("sending 2 messages") {
      auto res =
          client.send(zq::str_message("Hello"), zq::str_message("world"));
      REQUIRE(res);
      THEN("reading N = 2 works fine") {
        auto maybe_data = recv_n<2>(server);
        int count = 0;
        while (!maybe_data && count < 10) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_data = recv_n<2>(server);
          count++;
        }

        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->msg_count, 2);
        REQUIRE_EQ(data->result(), RecvResult::Ok);
      }
    }

    WHEN("sending 4 messages") {
      auto res = client.send(zq::str_message("Hello"), zq::str_message("World"),
                             zq::str_message("what's"), zq::str_message("up"));
      THEN("reading N = 2 produces an overflow") {
        auto maybe_data = recv_n<2>(server);
        int count = 0;
        // this repetition ..., I should all change ot await like apis. TODO
        while (!maybe_data && count < 10) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_data = recv_n<2>(server);
          count++;
        }

        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_GT(data->msg_count, 2);
        REQUIRE_EQ(data->result(), RecvResult::Overflow);

        AND_THEN("the rest of the message can be received") {
          // nothing is lost
          auto maybe_data2 = server.recv_all();
          REQUIRE(maybe_data2);
          REQUIRE(maybe_data2.has_value());
          auto& data2 = maybe_data2.value();
          REQUIRE_EQ(data2->size(), 2);
          std::string msg1 = zq::as_string(data2->at(0));
          std::string msg2 = zq::as_string(data2->at(1));
          REQUIRE_EQ(msg1, "what's");
          REQUIRE_EQ(msg2, "up");
        }
      }
    }

    WHEN("sending 1 message") {
      auto res = client.send(zq::str_message("Hello"));
      THEN("reading N = 2 produces an underflow") {
        auto maybe_data = recv_n<2>(server);
        int count = 0;
        // this repetition ..., I should all change ot await like apis. TODO
        while (!maybe_data && count < 10) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_data = recv_n<2>(server);
          count++;
        }

        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->msg_count, 1);
        REQUIRE_EQ(data->result(), RecvResult::Underflow);
      }
    }
  }
}
