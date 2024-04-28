#include <doctest/doctest.h>
#include <zq/zq.hpp>
// #include <zq/typename.hpp>
#include <chrono>
#include <thread>
#include "pingpong.pb.h"

SCENARIO("Make a hello world request reply") {
  GIVEN("a zq context that ") {
    auto context = zq::mk_context();

    WHEN("running the tests") {
      auto push =
          context->connect(zq::SocketType::PUSH, "tcp://localhost:5555");
      auto pull = context->bind(zq::SocketType::PULL, "tcp://*:5555");
      THEN("we should see the tests pass") {
        REQUIRE(push);
        REQUIRE(pull);
        // auto res = push->send(std::string_view("Hello world"));
        auto res = push->send("Hello world");
        REQUIRE(res);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // CHECK(*res == 11);
        auto reply = pull->recv();
        REQUIRE(reply);

        // MESSAGE("The type is: ", zq::msg_to<std::string>((*reply)[0]));
        // MESSAGE("The data size is: ", (*reply)[1].size());
        // MESSAGE("The data is: ", zq::msg_to<std::string>((*reply)[1]));

        // NOT yet, still char[N]
        // CHECK_EQ(zq::msg_to<std::string>((*reply)[0]), "std::string");
        CHECK_EQ(zq::msg_to<std::string>((*reply)[1]), "Hello world");
      }
    }
  }
}
