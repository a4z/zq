#include <doctest/doctest.h>
#include <algorithm>
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
        auto maybe_messages = server->recv_all();
        while (!maybe_messages) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_messages = server->recv_all();
        }
        REQUIRE_EQ(maybe_messages.value()->size(), 1);
        auto& message = maybe_messages.value()->at(0);
        auto sv = zq::as_string_view(message);
        REQUIRE_EQ(sv, message_str);
      }
    }
    AND_WHEN("sending several messages as one package") {
      std::vector<std::string> str_messages = {"Hello", "how", "are", "you"};
      std::vector<zq::Message> messages;
      std::transform(str_messages.begin(), str_messages.end(),
                     std::back_inserter(messages),
                     [](auto& str) { return zq::str_message(str); });

      auto res =
          client->send(messages[0], messages[1], messages[2], messages[3]);
      REQUIRE(res);

      THEN("the messages are received as a package") {
        auto maybe_messages = server->recv_all();
        while (!maybe_messages) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          maybe_messages = server->recv_all();
        }
        REQUIRE(*maybe_messages);
        const std::vector<zq::Message>& received_messages =
            *maybe_messages.value();
        REQUIRE_EQ(received_messages.size(), 4);
        std::vector<std::string> received_strings;
        std::transform(
            received_messages.begin(), received_messages.end(),
            std::back_inserter(received_strings),
            [](const zq::Message& message) { return zq::as_string(message); });
        REQUIRE_EQ(received_strings, str_messages);
      }
    }
  }
}
