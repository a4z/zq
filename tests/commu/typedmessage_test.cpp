#include <doctest/doctest.h>

#include <zq/message.hpp>
#include <zq/zq.hpp>

#include "pingpong.pb.h"

SCENARIO("Testing an empty typed message") {
  GIVEN("an empty message") {
    zq::TypedMessage m;
    WHEN("checking the type size") {
      THEN("the size of the type is 0") {
        REQUIRE_EQ(m.type.size(), 0);
      }
      THEN("the size of data is 0") {
        REQUIRE_EQ(m.payload.size(), 0);
      }
    }
  }
}

SCENARIO("Testing an string based typed message") {
  GIVEN("a string") {
    std::string str = "hello";
    WHEN("creating a typed message") {
      auto tm = zq::typed_message(str);
      THEN("the typed message can be restored as string") {
        auto restored = zq::restore_as<std::string>(tm);
        REQUIRE(restored);
        REQUIRE_EQ(*restored, str);
      }
      AND_THEN("restoring to an integer is not possible") {
        auto restored = zq::restore_as<int>(tm);
        REQUIRE_FALSE(restored);
      }
    }
  }
}

SCENARIO("Testing int based typed message") {
  GIVEN("some int16_t") {
    int16_t num = 42;
    WHEN("creating a typed message") {
      auto tm = zq::typed_message(num);
      THEN("the typed message can be restored as string") {
        auto restored = zq::restore_as<int16_t>(tm);
        REQUIRE(restored);
        REQUIRE_EQ(*restored, num);
      }
      AND_THEN("restoring to an integer is not possible") {
        auto restored = zq::restore_as<int>(tm);
        REQUIRE_FALSE(restored);
      }
    }
  }
}

SCENARIO("Testing double based typed message") {
  GIVEN("some double") {
    double num = 42.2;
    WHEN("creating a typed message") {
      auto tm = zq::typed_message(num);
      THEN("the typed message can be restored as string") {
        auto restored = zq::restore_as<double>(tm);
        REQUIRE(restored);
        REQUIRE_EQ(*restored, num);
      }
      AND_THEN("restoring to a float is not possible") {
        auto restored = zq::restore_as<float>(tm);
        REQUIRE_FALSE(restored);
      }
    }
  }
}

SCENARIO("Testing a protobuf based typed message") {
  GIVEN("some Protobuf") {
    zq::proto::Ping ping;
    ping.set_id(23);
    ping.set_msg("I am here");
    ;
    WHEN("creating a typed message") {
      auto tm = zq::typed_message(ping);
      THEN("the typed message can be restored as string") {
        auto restored = zq::restore_as<zq::proto::Ping>(tm);
        REQUIRE(restored);
        REQUIRE_EQ(restored->id(), ping.id());
        REQUIRE_EQ(restored->msg(), ping.msg());
      }
      AND_THEN("restoring to an other protobuf does not work") {
        auto restored = zq::restore_as<zq::proto::Pong>(tm);
        REQUIRE_FALSE(restored);
      }
    }
  }
}
