#include <doctest/doctest.h>
#include <zq/zq.hpp>

#include "pingpong.pb.h"

SCENARIO("Using protobuf") {
  GIVEN("a ping message") {
    zq::proto::Ping ping;

    WHEN("setting properties") {
      ping.set_id(1);
      ping.set_msg("hello");

      THEN("we should see the properties set") {
        CHECK(ping.id() == 1);
        CHECK(ping.msg() == "hello");
      }
    }
  }
}
