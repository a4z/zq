#include <doctest/doctest.h>
#include <zq/zq.hpp>
//#include <zq/typename.hpp>
#include "pingpong.pb.h"
#include <thread>
#include <chrono>

namespace {
  // startup times on Windows are a problem, they take too long,
  // this can cause test timeout
  using namespace std::chrono_literals;
  auto await_time = 1000ms;
  //auto await_time = std::chrono::milliseconds(1000);
}


struct MyStruct {
  int i{ 0 };
  float f{ 0.0f };
};
struct OtherStruct {
  int i{ 0 };
  float f{ 0.0f };
};

SCENARIO("Send a struct") {

  auto context = zq::mk_context();

  GIVEN("a push and a pull socket") {
    // auto push = context->connect(zq::SocketType::PUSH, "tcp://localhost:5555");
    // auto pull = context->bind(zq::SocketType::PULL, "tcp://localhost:5555");

    auto push = context->connect(zq::SocketType::PUSH, "ipc://push_pull_test_xteststr");
    auto pull = context->bind(zq::SocketType::PULL, "ipc://push_pull_test_xteststr");

    REQUIRE(push);
    REQUIRE(pull);

    WHEN("pushing a struct") {

      MyStruct s = { 42, 3.14f };
      auto tm = zq::typed_message(s);
      // MESSAGE("sending: " << as_string(tm.type)); -> MESSAGE: sending: MyStruct
      auto res = push->send(tm);
      REQUIRE(res);

      THEN("it's possible to receive and restore the struct") {
        auto reply = pull->await(await_time);
        REQUIRE(reply);
        auto restored = zq::restore_as<MyStruct>(*reply.value());
        REQUIRE(restored);
        auto received = *restored;
        REQUIRE_EQ(received.i, s.i);
        REQUIRE_EQ(received.f, s.f);
      }
      AND_THEN("it's not possible to receive and restore a different struct"){
        auto reply = pull->await(await_time);
        REQUIRE(reply);
        auto restored = zq::restore_as<OtherStruct>(*reply.value());
        REQUIRE_FALSE(restored);
      }
    }
  }
}

