#include <doctest/doctest.h>
#include <zq/zq.hpp>

SCENARIO("Identify an empty message") {
  GIVEN("a default constructed message") {
    zq::Message message;
    WHEN("checking the size of the message") {
      THEN("it is zero") {
        REQUIRE_EQ(message.size(), 0);
      }
    }
  }
}
