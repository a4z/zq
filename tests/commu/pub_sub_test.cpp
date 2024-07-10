#include <doctest/doctest.h>
#include "../zq_testing.hpp"

#include <thread>

namespace {
  // startup times on Windows are a problem, they take too long,
  // this can cause test timeout
  using namespace std::chrono_literals;
  auto await_time = 1000ms;
  // auto await_time = std::chrono::milliseconds(1000);
}  // namespace

// since this might be a useful scenario the api should support it

SCENARIO("Basic publish and scubscribe") {
  auto context = zq::mk_context();
  auto endpoint = next_ipc_address();
  ;

  GIVEN("publish and subscribe socket") {
    auto publisher = context->bind(zq::SocketType::PUB, endpoint);
    auto string_subscriber = context->connect(zq::SocketType::SUB, endpoint);
    REQUIRE(publisher);
    REQUIRE(string_subscriber);

    WHEN("subscribing the listener, and pushing a typed message") {
      using namespace std::chrono_literals;
      auto rc = zq::subscribe(*string_subscriber, {});
      REQUIRE(rc);
      // it takes time to submit the filter, todo, is there a way to signal when
      // a filter arrives?
      std::this_thread::sleep_for(50ms);

      auto tm = zq::typed_message("Hello world");
      auto send_rc = publisher->send(tm);
      REQUIRE(send_rc);

      THEN("it's possible to receive messages from the publishers") {
        auto reply = string_subscriber->await(await_time);
        REQUIRE(reply);
        REQUIRE(reply.value());
        auto restored = zq::restore_as<std::string>(*reply.value());
        REQUIRE_EQ(restored, "Hello world");
      }
    }
  }
}

SCENARIO("Publish and subscribe with filter") {
  auto context = zq::mk_context();
  auto endpoint = "ipc://test_pubsub1.ipc";

  GIVEN("publish and subscribe socket") {
    auto publisher = context->bind(zq::SocketType::PUB, endpoint);
    REQUIRE(publisher);
    auto string_subscriber = context->connect(zq::SocketType::SUB, endpoint);
    REQUIRE(string_subscriber);
    auto int_subscriber = context->connect(zq::SocketType::SUB, endpoint);
    REQUIRE(int_subscriber);

    WHEN(
        "subscribing the listener, and pushing A int and string typed "
        "message") {
      using namespace std::chrono_literals;
      // it takes time ...
      REQUIRE(zq::subscribe(*string_subscriber, {zq::str_type_name}));
      REQUIRE(zq::subscribe(*int_subscriber, {"int"}));
      std::this_thread::sleep_for(50ms);

      REQUIRE(publisher->send(zq::typed_message("Hello world")));
      REQUIRE(publisher->send(zq::typed_message(int{42})));

      THEN("the subscribers get only their messages") {
        auto string_reply = string_subscriber->await(await_time);
        auto int_reply = int_subscriber->await(await_time);
        REQUIRE(string_reply);
        REQUIRE(string_reply.value());
        auto restored_string =
            zq::restore_as<std::string>(*string_reply.value());
        REQUIRE_EQ(restored_string, "Hello world");

        REQUIRE(int_reply);
        REQUIRE(int_reply.value());
        auto restored_int = zq::restore_as<int>(*int_reply.value());
        REQUIRE_EQ(restored_int, 42);

        AND_THEN("all messages have been received") {
          REQUIRE_FALSE(string_subscriber->await(100ms));
          REQUIRE_FALSE(int_subscriber->await(100ms));
        }
      }
    }
  }
}

SCENARIO("Wrong subscriber call") {
  auto context = zq::mk_context();
  REQUIRE(context);

  GIVEN("a request, a reply socket") {
    auto [client, server] = pp_cs_sockets(*context, next_inproc_address());

    WHEN("trying to subscribe the push socket") {
      auto subscribe_rc = zq::subscribe(client, {"foo", "bar"});

      THEN("an error is returned") {
        REQUIRE_FALSE(subscribe_rc);
        zq::Error err = subscribe_rc.error();
        // this is a logical error on our side, not a internal ZMQ error
        REQUIRE(err.isZqError());
        REQUIRE_FALSE(err.isZmqError());
        const std::string err_msg = err.what();
        REQUIRE(doctest::Contains{"not a subscr"}.checkWith(err_msg.c_str()));

      }
    }
  }
}
