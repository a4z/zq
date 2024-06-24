#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <zq/zq.hpp>

std::tuple<zq::Socket, zq::Socket> pp_cs_sockets(zq::Context& context,
                                                 std::string_view address) {
  auto server = context.bind(zq::SocketType::REP, address);
  auto client = context.connect(zq::SocketType::REQ, address);
  REQUIRE(client);
  REQUIRE(server);
  return {std::move(*client), std::move(*server)};
}

enum class RecvResult { Ok, Underflow, Overflow };

template <size_t N>
struct RecvData {
  size_t msg_count{0};
  std::array<zq::Message, N> messages{};

  RecvResult result() {
    if (msg_count < N) {
      return RecvResult::Underflow;
    }
    if (msg_count > N) {
      return RecvResult::Overflow;
    }
    return RecvResult::Ok;
  }
};

template <size_t N>
auto recv_n(zq::Socket& socket)
    -> std::optional<tl::expected<RecvData<N>, std::runtime_error>> {
  RecvData<N> data{};
  auto* socket_ptr = socket.socket_ptr.get();
  using zq::currentZmqRuntimeError;

  auto rc = zmq_msg_recv(std::addressof(data.messages[0].msg), socket_ptr,
                         ZMQ_DONTWAIT);

  if (rc == -1) {
    if (zmq_errno() == EAGAIN) {
      return std::nullopt;
    } else {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
  }
  data.msg_count++;

  for (size_t i = 1; i < N; ++i) {
    int more = 0;
    size_t more_size = sizeof(more);
    rc = zmq_getsockopt(socket_ptr, ZMQ_RCVMORE, &more, &more_size);
    if (rc == -1) {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
    if (!more) {
      break;
    }
    rc = zmq_msg_recv(std::addressof(data.messages[i].msg), socket_ptr, 0);
    if (rc == -1) {
      return tl::make_unexpected(currentZmqRuntimeError());
    }
    data.msg_count++;
  }
  return data;
}

SCENARIO("Make a hello world send receive call") {
  auto context = zq::mk_context();
  REQUIRE(context);

  GIVEN("a request, a reply socket, and an untyped message") {
    std::string_view address = "ipc://localhost_5556";
    auto [client, server] = pp_cs_sockets(*context, address);

    WHEN("sending the simple message") {
      THEN("it is not possible to receive this as a typed message") {}
    }
  }
}
