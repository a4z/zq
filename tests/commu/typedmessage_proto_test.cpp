#include <doctest/doctest.h>

#include <zq/zq.hpp>
#include "pingpong.pb.h"

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
