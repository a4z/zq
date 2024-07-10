#include <doctest/doctest.h>

#include <zq/message.hpp>
#include <zq/zq.hpp>

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
        auto restored = zq::restore_as<std::string>(tm);
        REQUIRE_FALSE(restored);
        auto err = restored.error();
        REQUIRE_EQ(std::string(err.what()), "Message type does not match");
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
    AND_WHEN("Move assign the message") {
      auto tm = zq::typed_message(num);
      auto tm2 = zq::typed_message(int{12});
      tm2 = std::move(tm);
      THEN("the message can be restored as expected") {
        auto restored = zq::restore_as<double>(tm2);
        REQUIRE(restored);
        REQUIRE_EQ(*restored, num);
      }
    }
  }
}
