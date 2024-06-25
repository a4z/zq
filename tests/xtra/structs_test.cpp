#include <doctest/doctest.h>
#include "../zq_testing.hpp"
// #include <zq/typename.hpp>
#include <chrono>
#include <thread>

namespace {
  // startup times on Windows are a problem, they take too long,
  // this can cause test timeout
  using namespace std::chrono_literals;
  auto await_time = 1000ms;
  // auto await_time = std::chrono::milliseconds(1000);
}  // namespace

struct MyStruct {
  int i{0};
  float f{0.0f};
};
struct OtherStruct {
  int i{0};
  float f{0.0f};
};

SCENARIO("Send a struct") {
  auto context = zq::mk_context();

  GIVEN("a push and a pull socket") {
    const auto address = next_ipc_address();
    auto push = context->connect(zq::SocketType::PUSH, address);
    auto pull = context->bind(zq::SocketType::PULL, address);

    REQUIRE(push);
    REQUIRE(pull);

    WHEN("pushing a struct") {
      MyStruct s = {42, 3.14f};
      auto tm = zq::typed_message(s);
      // MESSAGE("sending: " << as_string(tm.type)); -> MESSAGE: sending:
      // MyStruct
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
      AND_THEN("it's not possible to receive and restore a different struct") {
        auto reply = pull->await(await_time);
        REQUIRE(reply);
        auto restored = zq::restore_as<OtherStruct>(*reply.value());
        REQUIRE_FALSE(restored);
      }
    }
  }
}
