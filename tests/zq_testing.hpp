#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <zq/zq.hpp>

inline std::tuple<zq::Socket, zq::Socket> pp_cs_sockets(
    zq::Context& context,
    std::string_view address) {
  auto server = context.bind(zq::SocketType::REP, address);
  auto client = context.connect(zq::SocketType::REQ, address);
  REQUIRE(client);
  REQUIRE(server);
  return {std::move(*client), std::move(*server)};
}

inline std::atomic<int> atomic_counter{0};

inline std::string next_inproc_address() {
  return "inproc://#" + std::to_string(atomic_counter++);
}

inline std::string next_ipc_address() {
  return "ipc://zq_testpipe_" + std::to_string(atomic_counter++);
}

// if I would always run ctest, it would not be required
struct TimeOutInsurance {
  using milliseconds = std::chrono::milliseconds;

  // no copy operations
  TimeOutInsurance(const TimeOutInsurance&) = delete;
  TimeOutInsurance& operator=(const TimeOutInsurance&) = delete;

  TimeOutInsurance(milliseconds timeout)
      : t([this, timeout] {
          auto end_time = std::chrono::steady_clock::now() + timeout;
          for (;;) {
            std::this_thread::sleep_for(milliseconds{10});
            if (this->done) {
              return;
            }
            if (std::chrono::steady_clock::now() > end_time) {
              std::exit(EXIT_FAILURE);
            }
          }
        }) {}

  ~TimeOutInsurance() {
    done = true;
    // one day, also apple clang will support jthread ....
    if (t.joinable()) {
      t.join();
    }
  }

 private:
  std::atomic_bool done{false};
  std::thread t;
};
