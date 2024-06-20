#include <doctest/doctest.h>
#include <zq/zq.hpp>
#include <fmt/format.h>
#include <chrono>
#include <thread>
#include "pingpong.pb.h"

namespace {
  // startup times on Windows are a problem, they take too long,
  // this can cause test timeout
  using namespace std::chrono_literals;
  auto await_time = 1000ms;
  // auto await_time = std::chrono::milliseconds(1000);
}  // namespace


SCENARIO("Make a hello world send receive call with proto messages") {
  auto context = zq::mk_context();

  GIVEN("a request and a reply socket with proto messages") {
    // auto client = context->connect(zq::SocketType::REQ,
    // "tcp://localhost:5555"); auto server = context->bind(zq::SocketType::REP,
    // "tcp://localhost:5555");
    auto client = context->connect(zq::SocketType::REQ, "ipc://localhost_5555");
    auto server = context->bind(zq::SocketType::REP, "ipc://localhost_5555");
    REQUIRE(client);
    REQUIRE(server);

    WHEN("requesting a ping reply") {
      zq::proto::Ping ping;
      ping.set_id(1);
      ping.set_msg("Hi");
      ;
      auto tm = zq::typed_message("Hello world");
      auto res = client->send(zq::typed_message(ping));
      REQUIRE(res);

      THEN("it's possible to receive and restore the message") {
        auto request = server->await(await_time);
        REQUIRE(request);
        REQUIRE(request.value());
        auto restored = zq::restore_as<zq::proto::Ping>(*request.value());
        if (!restored) {
          MESSAGE("error: ", restored.error().what());
        }
        REQUIRE(restored);
        REQUIRE_EQ(restored->id(), 1);
        REQUIRE_EQ(restored->msg(), "Hi");
        AND_THEN("it's possible to send and receive a reply") {
          zq::proto::Pong pong;
          pong.set_id(1);
          pong.set_reply("Hi there");
          auto send_rc = server->send(zq::typed_message(pong));
          REQUIRE(send_rc);
          auto reply = client->await(await_time);
          REQUIRE(reply);
          REQUIRE(reply.value());
          auto restored_reply = zq::restore_as<zq::proto::Pong>(*reply.value());
          REQUIRE_EQ(restored_reply->id(), 1);
          REQUIRE_EQ(restored_reply->reply(), "Hi there");
        }
      }
    }
  }
}
