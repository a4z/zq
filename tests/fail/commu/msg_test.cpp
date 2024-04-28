#include <doctest/doctest.h>
#include <zq/zq.hpp>

#include "pingpong.pb.h"

#include <zq/msg.hpp>

SCENARIO("Testing an empty message") {
  GIVEN("a default constructed Msg") {
    zq::Msg m;
    WHEN("checking the message type") {
      THEN("the message type is empty") {
        REQUIRE(m.type() == zq::MsgType::Empty);
      }
    }
  }
}

SCENARIO("Testing a Msg with a string") {
  GIVEN("a message with a std::string") {
    std::string str = "Hello World";
    auto m = zq::mk_msg(str);
    WHEN("checking the message we got") {
      THEN("the message is not empty") {
        REQUIRE(m);
      }
      AND_THEN("the message has type MsgType::Variant") {
        REQUIRE(m->type() == zq::MsgType::Variant);
      }
      AND_THEN("the string can be restored from a message") {
        auto restored = zq::msg_as<std::string>(*m);
        REQUIRE(restored);
        REQUIRE(restored.value() == str);
      }
    }
  }

  GIVEN("a message with a char array") {
    auto m = zq::mk_msg("Hello World");
    WHEN("checking the message we got") {
      THEN("the message is not empty") {
        REQUIRE(m);
      }
      AND_THEN("the message has type MsgType::Variant") {
        REQUIRE(m->type() == zq::MsgType::Variant);
      }
    }
  }
}

SCENARIO("Testing a Msg with an int") {
  GIVEN("a message with an int") {
    auto m = zq::mk_msg(24);
    WHEN("checking the message we got") {
      THEN("the message is not empty") {
        REQUIRE(m);
      }
      AND_THEN("the message has type MsgType::Variant") {
        REQUIRE(m->type() == zq::MsgType::Variant);
      }
    }
  }
}
