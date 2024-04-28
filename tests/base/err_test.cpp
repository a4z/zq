#include <doctest/doctest.h>
#include <zq/zq.hpp>

SCENARIO("Testing the Error type") {
  GIVEN("an Error instances with value 0") {
    zq::Error e0{0};
    WHEN("comparing ") {
      THEN("with NoError, it is no error") {
        CHECK(e0 == zq::NoError);
      }
      AND_THEN("it can also be compared with an integer") {
        CHECK(e0 == 0);
      }
      AND_THEN("it can be used as a boolean") {
        CHECK_FALSE(e0);
      }
    }
  }
  GIVEN("an Error instances with value 1") {
    zq::Error e1{1};
    WHEN("comparing") {
      THEN("with NoError, it is no error") {
        CHECK_FALSE(e1 == zq::NoError);
      }
      AND_THEN("with an integer, it works") {
        CHECK_NE(e1, 0);
        CHECK_EQ(e1, 1);
      }
      AND_THEN("it can be used as a boolean") {
        CHECK(e1);
      }
    }
  }
}
