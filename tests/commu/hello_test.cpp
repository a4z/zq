#include <doctest/doctest.h>
#include <chrono>
#include <thread>
#include <zq/zq.hpp>

namespace {
  // startup times on Windows are a problem, they take too long,
  // this can cause test timeout
  using namespace std::chrono_literals;
  auto await_time = 1000ms;
  // auto await_time = std::chrono::milliseconds(1000);
}  // namespace

SCENARIO("Make a hello world send receive call") {
  auto context = zq::mk_context();

  GIVEN("a push and a pull socket") {
    // auto push = context->connect(zq::SocketType::PUSH,
    // "tcp://localhost:5555"); auto pull = context->bind(zq::SocketType::PULL,
    // "tcp://localhost:5555");

    auto push =
        context->connect(zq::SocketType::PUSH, "ipc://push_pull_test_1");
    auto pull = context->bind(zq::SocketType::PULL, "ipc://push_pull_test_1");

    REQUIRE(push);
    REQUIRE(pull);

    WHEN("pushing a typed message") {
      auto tm = zq::typed_message("Hello world");

      auto type_name = zq::as_string(tm.type);
      std::string wanted = zq::str_type_name;
      REQUIRE_EQ(type_name.size(), wanted.size());
      REQUIRE_EQ(type_name, wanted);
      auto res = push->send(tm);
      REQUIRE(res);

      THEN("it's possible to receive and restore the message") {
        // don't do this, use await instead, it allows doing lang breaks

        // to not try to sleep so long,
        std::string received_string = "";
        for (int i = 0; i < 10; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          auto reply = pull->recv();
          if (reply) {
            auto restored = zq::restore_as<std::string>(*reply.value());
            REQUIRE(restored);
            if (restored) {
              received_string = *restored;
              break;
            }
          }
        }
        REQUIRE_EQ(received_string, "Hello world");
      }
      // NOTE: for testing purpose, always await should be used, to not
      // pointless sleep for no reason
      AND_THEN("it's also possible to await a message for some time") {
        auto reply = pull->await(await_time);
        REQUIRE(reply);
        REQUIRE(reply.value());
        auto restored = zq::restore_as<std::string>(*reply.value());
        REQUIRE_EQ(restored, "Hello world");
      }
    }
    AND_WHEN("No message was sent") {
      // .. nothing happes :-)
      THEN("receive and await should return nothing") {
        auto reply = pull->await(std::chrono::milliseconds{100});
        REQUIRE_FALSE(reply);
        auto reply2 = pull->recv();
        REQUIRE_FALSE(reply2);
      }
    }
  }
}

SCENARIO("Make a hello world send receive call") {
  auto context = zq::mk_context();

  GIVEN("a request and a reply socket int message") {
    // auto client = context->connect(zq::SocketType::REQ,
    // "tcp://localhost:5555"); auto server = context->bind(zq::SocketType::REP,
    // "tcp://localhost:5555");
    auto client = context->connect(zq::SocketType::REQ, "ipc://localhost_5555");
    auto server = context->bind(zq::SocketType::REP, "ipc://localhost_5555");
    REQUIRE(client);
    REQUIRE(server);

    WHEN("requesting a ping reply") {
      std::string hello_str = "Hello world";
      auto tm = zq::typed_message(hello_str);
      auto res = client->send(tm);
      REQUIRE(res);

      THEN("it's possible to receive and restore the message") {
        auto request = server->await(await_time);
        REQUIRE(request);
        REQUIRE(request.value());
        auto restored = zq::restore_as<std::string>(*request.value());
        if (!restored) {
          MESSAGE("error: ", restored.error().what());
        }
        REQUIRE(restored);
        REQUIRE_EQ(*restored, hello_str);

        AND_THEN("it's possible to send and receive a reply") {
          std::string reply_str = "Hi there";
          auto send_rc = server->send(zq::typed_message(reply_str));
          REQUIRE(send_rc);
          auto reply = client->await(await_time);
          REQUIRE(reply);
          REQUIRE(reply.value());
          auto restored_reply = zq::restore_as<std::string>(*reply.value());
          REQUIRE(restored_reply);
          REQUIRE_EQ(*restored_reply, reply_str);
        }
      }
    }
  }
}

SCENARIO(
    "Trying to create a socket without valid context returns error message") {
  GIVEN("The invalid ZMQ context") {
    auto maybe_context = zq::mk_context();

    REQUIRE(maybe_context.has_value());
    REQUIRE(maybe_context->close() == zq::NoError);

    WHEN("Trying to create a (Request) socket") {
      auto maybe_socket{
          maybe_context->connect(zq::SocketType::REQ, "ipc://localhost_5555")};

      THEN("The error message is returned") {
        REQUIRE_FALSE(maybe_socket.has_value());
        REQUIRE(doctest::Contains{"Bad address"}.checkWith(
            fmt::format("{}", maybe_socket.error()).c_str()));
      }
    }
  }
}
