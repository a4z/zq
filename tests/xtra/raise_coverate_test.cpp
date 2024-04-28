#include <doctest/doctest.h>
#include <zq/zq.hpp>

SCENARIO("Closing a socket multiple times") {

  auto context = zq::mk_context();

  GIVEN("a zq socket") {
    auto push = context->connect(zq::SocketType::PUSH, "ipc://close_test1");

    WHEN("closing the socket two times") {
      auto res1 = push->close();
      auto res2 = push->close();
      THEN("no error happens") {
        CHECK(res1 == zq::NoError);
        CHECK(res2 == zq::NoError);
      }
    }
    AND_WHEN("closing the socket by bypassing the close method") {
      // this will also trigger coverage for the deleter,
      // which is otherwise never triggered du to the reset
      push->socket_ptr.get_deleter()(push->socket_ptr.get());
      auto res = push->close();
      THEN("closing the socket will cause an error") {
        CHECK(res != zq::NoError);
      }
    }
  }
}
