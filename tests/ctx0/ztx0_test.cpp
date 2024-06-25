#include <doctest/doctest.h>
#include <zq/zq.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>
#include "../zq_testing.hpp"

// TODO, move this somewhere else

SCENARIO("Working without zmq background thread") {
  // NOTE: this is only possible with inproc sockets !!

  auto inproc_adr = next_inproc_address();

  auto Options = {
      zq::ContextOption{.name = zq::CtxOptionName::IO_THREADS, .value = 0},
      zq::ContextOption{zq::CtxOptionName::BLOCKY, 0},
  };

  auto context = zq::mk_context(Options);
  using namespace std::chrono_literals;
  TimeOutInsurance toi{1000ms};

  GIVEN("A push and a pull thread") {
    std::atomic_bool received{false};

    std::thread puller([&] {
      auto pull = context->bind(zq::SocketType::PULL, inproc_adr);
      REQUIRE(pull);
      auto reply = pull->await(1000ms);
      REQUIRE(reply);
      REQUIRE(reply.value());
      auto restored = zq::restore_as<std::string>(*reply.value());
      REQUIRE(restored);
      REQUIRE_EQ(*restored, "Hello world");
      received = true;
    });

    std::thread pusher([&] {
      auto push = context->connect(zq::SocketType::PUSH, inproc_adr);
      REQUIRE(push);
      auto tm = zq::typed_message("Hello world");
      auto res = push->send(tm);
      REQUIRE(res);
      while (!received) {
        std::this_thread::sleep_for(10ms);
      }
    });

    WHEN("joining the threads") {
      if (pusher.joinable()) {
        pusher.join();
      }
      if (puller.joinable()) {
        puller.join();
      }
      THEN("the message eventually arrives") {
        REQUIRE(received);
      }
    }
  }
}
