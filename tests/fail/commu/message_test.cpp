#include <doctest/doctest.h>
#include <zq/zq.hpp>

#include "pingpong.pb.h"

SCENARIO("Testing the message class") {
  GIVEN("some string") {
    std::string str = "Hello World";
    WHEN("we create a message from the string") {
      zq::Message msg{str};
      THEN("we should see the tests pass") {
        CHECK(msg_to<std::string>(msg) == str);
      }
    }
  }

  GIVEN("some int32") {
    int32_t data = 42;
    WHEN("we create a message from the number") {
      zq::Message msg{data};
      THEN("we should see the tests pass") {
        CHECK(msg_to<int32_t>(msg).value() == data);
      }
    }
    AND_WHEN("converting to a different sized type") {
      auto expected = msg_to<int64_t>(zq::Message{data});
      THEN("We have an exception") {
        CHECK_FALSE(expected);
      }
    }
  }

  GIVEN("a proto buf message") {
    zq::proto::Ping ping;
    ping.set_id(23);
    ping.set_msg("I am here");
    WHEN("we create a message from the message") {
      zq::Message msg{ping};
      THEN("we should see the tests pass") {
        auto ping_back = msg_to<zq::proto::Ping>(msg);
        REQUIRE(ping_back);
        CHECK(ping_back->id() == 23);
        CHECK(ping_back->msg() == "I am here");
      }
    }
    // That's not idiot proof, might be the ping message has the same size ...
    AND_WHEN("converting to a different type") {
      auto expected = msg_to<int64_t>(zq::Message{ping});
      THEN("We have an exception") {
        CHECK_FALSE(expected);
      }
    }
  }
}
