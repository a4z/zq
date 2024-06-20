#include <doctest/doctest.h>
#include <zq/zq.hpp>

SCENARIO("Make a hello world send receive call") {
  auto context = zq::mk_context();

  GIVEN("a request, a reply socket, and an untyped message") {

    std::string_view address = "ipc://localhost_5556";

    auto client = context->connect(zq::SocketType::REQ,address);
    auto server = context->bind(zq::SocketType::REP, address);
    REQUIRE(client);
    REQUIRE(server);

    auto simple_message = zq::str_message("Hello world");

    WHEN("sending the simple message") {

      auto res = client->sendMsg(simple_message);
      REQUIRE(res);

      THEN("it is not possible to receive this as a typed message") {
        auto request = server->recv();
        REQUIRE_FALSE(request);
      }
    }
  }
}
