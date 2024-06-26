#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

#include "../zq_testing.hpp"

SCENARIO("Testing recv_n") {
  auto context = zq::mk_context();
  REQUIRE(context);
  using namespace std::chrono_literals;

  GIVEN("a request, a reply socket") {
    auto [client, server] = pp_cs_sockets(*context, next_inproc_address());

    WHEN("sending 2 messages") {
      auto res =
        client.send(zq::str_message("Hello"), zq::str_message("world"));
      REQUIRE(res);
      THEN("reading N = 2 works fine") {
        auto poll_rc = server.poll(500ms);
        REQUIRE(poll_rc);
        REQUIRE_EQ(poll_rc.value(), true);
        auto maybe_data = server.recv_n<2>();
        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->msg_count, 2);
        REQUIRE_EQ(data->result(), zq::RecvResult::Ok);
      }
    }

    WHEN("sending 4 messages") {
      auto res = client.send(zq::str_message("Hello"), zq::str_message("World"),
        zq::str_message("what's"), zq::str_message("up"));
      THEN("reading N = 2 produces an overflow") {
        auto poll_rc = server.poll(500ms);
        REQUIRE(poll_rc);
        REQUIRE_EQ(poll_rc.value(), true);
        auto maybe_data = server.recv_n<2>();
        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_GT(data->msg_count, 2);
        REQUIRE_EQ(data->result(), zq::RecvResult::Overflow);

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
        auto poll_rc = server.poll(500ms);
        REQUIRE(poll_rc);
        REQUIRE_EQ(poll_rc.value(), true);
        auto maybe_data = server.recv_n<2>();
        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->msg_count, 1);
        REQUIRE_EQ(data->result(), zq::RecvResult::Underflow);
      }
    }
  }
}


SCENARIO("Send a container of messages") {
  auto context = zq::mk_context();
  REQUIRE(context);
  using namespace std::chrono_literals;

  GIVEN("a request, a reply socket and a vector of messages") {
    auto [client, server] = pp_cs_sockets(*context, next_inproc_address());
    std::vector<zq::Message> msgs;
    msgs.push_back(zq::str_message("Hello"));
    msgs.push_back(zq::str_message("World"));
    msgs.push_back(zq::str_message("What's"));
    msgs.push_back(zq::str_message("Up"));

    WHEN("sending the vector of messages") {
      auto res = client.send(msgs);
      REQUIRE(res);
      THEN("reading all data works fine") {
        auto poll_rc = server.poll(500ms);
        REQUIRE(poll_rc);
        REQUIRE_EQ(poll_rc.value(), true);
        auto maybe_data = server.recv_all();
        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->size(), 4);
        std::string msg1 = zq::as_string(data->at(0));
        std::string msg3 = zq::as_string(data->at(3));
        REQUIRE_EQ(msg1, "Hello");
        REQUIRE_EQ(msg3, "Up");
      }
    }

    AND_WHEN("sending an array of messages") {
      std::array<zq::Message, 4> amsgs = {{ zq::str_message("Hello"),
                                          zq::str_message("World"),
                                          zq::str_message("What's"),
                                          zq::str_message("Up")}};
      auto res = client.send(amsgs);
      REQUIRE(res);
      THEN("reading all data works fine") {
        auto poll_rc = server.poll(500ms);
        REQUIRE(poll_rc);
        REQUIRE_EQ(poll_rc.value(), true);
        auto maybe_data = server.recv_all();
        REQUIRE(maybe_data);
        REQUIRE(maybe_data.has_value());
        auto& data = maybe_data.value();
        REQUIRE_EQ(data->size(), 4);
        std::string msg1 = zq::as_string(data->at(0));
        std::string msg3 = zq::as_string(data->at(3));
        REQUIRE_EQ(msg1, "Hello");
        REQUIRE_EQ(msg3, "Up");
      }
    }

  }
}
