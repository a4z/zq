#pragma once

#include "a4z/typename.hpp"
#include "config.hpp"
#include "message.hpp"
#include "zflags.hpp"

#include <chrono>
#include <string_view>

namespace zq {

  // Since socket resets the pointer, this is never called in zq code
  // but to be technically correct, this deleter is needed for the unique_ptr
  struct ZmqSocketClose {
    void operator()(void* ptr) const {
      if (ptr) {
        zmq_close(ptr);
      }
    }
  };

  using SocketPointer = std::unique_ptr<void, ZmqSocketClose>;

  struct Socket {
    SocketPointer socket_ptr{nullptr};

    Socket(SocketPointer socket) noexcept : socket_ptr{std::move(socket)} {};

    Socket(Socket&& rhs) noexcept : socket_ptr{std::move(rhs.socket_ptr)} {}

    Socket(Socket const&) = delete;
    Socket& operator=(Socket const&) = delete;
    Socket& operator=(Socket&&) = delete;

    ~Socket() noexcept { [[maybe_unused]] auto _ = close(); }

    [[nodiscard]] Error close() noexcept {
      if (socket_ptr != nullptr) {
        if (zmq_close(socket_ptr.get()) != 0) {
          return Error{zmq_errno()};
        }
        [[maybe_unused]] auto _ = socket_ptr.release();
        socket_ptr = nullptr;
      }
      return NoError;
    }

    template <pack_of_messages... Messages>
    [[nodiscard]] tl::expected<size_t, ErrMsg> sendMsg(
        [[maybe_unused]] const Message& first,
        const Messages&... messages) {
      size_t bytes_sent = 0;

      if constexpr (sizeof...(messages) == 0) {
        auto rc = zmq_send(socket_ptr.get(), first.data(), first.size(),
                           ZMQ_DONTWAIT);
        if (rc < 0) {
          return tl::make_unexpected(currentErrMsg());
        }
        bytes_sent += static_cast<size_t>(rc);
      } else {
        auto rc =
            zmq_send(socket_ptr.get(), first.data(), first.size(), ZMQ_SNDMORE);
        if (rc < 0) {
          return tl::make_unexpected(currentErrMsg());
        }
        auto maybe_rc = sendMsg(messages...);
        if (!maybe_rc) {
          return maybe_rc;
        }
        bytes_sent += static_cast<size_t>(rc) + maybe_rc.value();
      }
      return bytes_sent;
    }

    /**
     * @brief Send a typed message
     *
     * @param msg
     * @return tl::expected<size_t, ErrMsg>
     */
    [[nodiscard]] tl::expected<size_t, ErrMsg> send(const TypedMessage& msg) {
      return sendMsg(msg.type, msg.payload);
    }

    // either error, no message available, or message
    // that's hard to model, but maybe that works
    // but it might be annoying to check every non receive for an error
    // so still an optional ?
    [[nodiscard]]
    // tl::expected<TypedMessage, std::optional<std::runtime_error>>
    std::optional<tl::expected<TypedMessage, std::runtime_error>>
    recv() {
      TypedMessage typed_message;

      auto rc = zmq_msg_recv(std::addressof(typed_message.type.msg),
                             socket_ptr.get(), ZMQ_DONTWAIT);

      if (rc == -1) {
        if (zmq_errno() == EAGAIN) {
          return std::nullopt;
        } else {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
      }
      int more = 0;
      size_t more_size = sizeof(more);
      rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
      if (rc == -1) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      if (more) {
        rc = zmq_msg_recv(std::addressof(typed_message.payload.msg),
                          socket_ptr.get(), 0);
        if (rc == -1) {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
      } else {
        return tl::make_unexpected(std::runtime_error("no more message"));
      }

      return typed_message;
    }

    [[nodiscard]] std::optional<tl::expected<TypedMessage, std::runtime_error>>
    await(std::chrono::milliseconds timeout) {
      zmq_pollitem_t poll_item[] = {{socket_ptr.get(), 0, ZMQ_POLLIN, 0}};
      long tm = static_cast<long>(timeout.count());
      auto rc = zmq_poll(poll_item, 1, tm);
      if (rc == -1) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }

      return recv();
    }
  };

  [[nodiscard]] inline tl::expected<size_t, std::runtime_error> subscribe(
      Socket& subscriber,
      std::initializer_list<std::string_view> topic_filters) {
    // subscribe to all topics, todo, maybe overload this function with 1 or 2
    // arguments
    {
      int val = 0;
      size_t size = sizeof(val);
      int rc = zmq_getsockopt(subscriber.socket_ptr.get(), ZMQ_TYPE,
                              std::addressof(val), std::addressof(size));
      if (rc != 0) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      if (val != ZMQ_SUB) {
        return tl::make_unexpected(
            std::runtime_error("socket is not a subscriber"));
      }
    }

    if (topic_filters.size() == 0) {
      auto rc =
          zmq_setsockopt(subscriber.socket_ptr.get(), ZMQ_SUBSCRIBE, "", 0);
      if (rc != 0) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      return std::numeric_limits<size_t>::max();
    }
    // subscribe to requested topics
    size_t topicCount = 0;
    for (auto&& t : topic_filters) {
      auto rc = zmq_setsockopt(subscriber.socket_ptr.get(), ZMQ_SUBSCRIBE,
                               t.data(), t.size());
      if (rc != 0) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      topicCount++;
    }
    return topicCount;
  }

}  // namespace zq
