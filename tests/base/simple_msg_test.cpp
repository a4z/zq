#include <doctest/doctest.h>
#include <chrono>
#include <thread>
#include <zq/zq.hpp>

SCENARIO("Make a hello world send receive call") {
  auto context = zq::mk_context();

  GIVEN("a request, a reply socket, and an untyped message") {
    std::string_view address = "ipc://localhost_5556";

    auto server = context->bind(zq::SocketType::REP, address);
    auto client = context->connect(zq::SocketType::REQ, address);
    REQUIRE(client);
    REQUIRE(server);
    auto message_str = "Hello world";
    auto simple_message = zq::str_message(message_str);

    WHEN("sending the simple message") {
      auto res = client->send(simple_message);
      REQUIRE(res);

      THEN("it is not possible to receive this as a typed message") {
        auto request = server->recv();
        while (!request) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          request = server->recv();
        }
        REQUIRE_FALSE(*request);
      }
      AND_THEN("it is possible to receive this as an untyped message") {
        auto maybe_messages = server->recv_n();
        while (!maybe_messages) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_messages = server->recv_n();
        }
        REQUIRE_EQ(maybe_messages.value()->size(), 1);
        auto& message = maybe_messages.value()->at(0);
        auto sv = zq::as_string_view(message);
        REQUIRE_EQ(sv, message_str);
      }
    }
  }
}
