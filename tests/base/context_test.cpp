#include <doctest/doctest.h>
#include <zq/zq.hpp>

SCENARIO ("We compile and run the tests") {

  GIVEN ("a zq context") {

    auto context = zq::mk_context();

    WHEN ("running the tests") {

      THEN ("we should see the tests pass") {
        CHECK (context);
      }
    }
  }
}
