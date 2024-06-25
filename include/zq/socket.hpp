#pragma once

#include "a4z/typename.hpp"
#include "config.hpp"
#include "message.hpp"
#include "zflags.hpp"

#include <array>
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

  using SocketPointer = std::unique_ptr<void, ZmqSocketClose>;

  struct Socket {
    SocketPointer socket_ptr{nullptr};

    Socket(SocketPointer socket) noexcept : socket_ptr{std::move(socket)} {};

    Socket(Socket&& rhs) noexcept : socket_ptr{std::move(rhs.socket_ptr)} {}

    Socket(Socket const&) = delete;
    Socket& operator=(Socket const&) = delete;
    Socket& operator=(Socket&&) = delete;

    ~Socket() noexcept { [[maybe_unused]] auto _ = close(); }

    /**
     * @brief Close the socket
     *
     * @return Error code
     */
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

    /**
     * @brief Send one or more messages
     *
     * @tparam Messages
     * @param first
     * @param messages
     * @return tl::expected<size_t, ErrMsg> Number of bytes sent, or an error
     */
    template <pack_of_messages... Messages>
    [[nodiscard]] tl::expected<size_t, ErrMsg> send(
        const Message& first,
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
        auto maybe_rc = send(messages...);
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
      return send(msg.type, msg.payload);
    }

    /**
     * @brief Return a typed message
     *
     *  Returns nothing if there is a zmq error
     *  Returns an error if it does not look like a typed message
     *
     * @return std::optional<tl::expected<TypedMessage, std::runtime_error>>
     */
    [[nodiscard]] std::optional<tl::expected<TypedMessage, std::runtime_error>>
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

    /**
     * @brief Receive as many message parts as there are on the socket
     *
     * @return std::optional<tl::expected<Message, std::runtime_error>>
     */
    [[nodiscard]] std::optional<
        tl::expected<std::vector<Message>, std::runtime_error>>
    recv_all() {
      std::vector<Message> messages;
      int more = 1;
      size_t more_size = sizeof(more);
      while (more) {
        Message message;
        auto rc = zmq_msg_recv(std::addressof(message.msg), socket_ptr.get(),
                               ZMQ_DONTWAIT);
        messages.emplace_back(std::move(message));
        if (rc == -1) {
          if (zmq_errno() == EAGAIN) {
            return std::nullopt;
          } else {
            return tl::make_unexpected(currentZmqRuntimeError());
          }
        }
        rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
        if (rc == -1) {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
      }
      return messages;
    }

    /**
     * @brief await a typed message
     *
     * This function is going to be replaced
     *
     * @param timeout
     * @return std::optional<tl::expected<TypedMessage, std::runtime_error>>
     */
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

    /**
     * @brief Poll for given timeout
     *
     * Returns true if there is data to receive, false otherwise
     *
     * @param timeout
     * @return tl::expected<bool, std::runtime_error>
     */
    [[nodiscard]] tl::expected<bool, std::runtime_error> poll(
        std::chrono::milliseconds timeout) {
      zmq_pollitem_t poll_item[] = {{socket_ptr.get(), 0, ZMQ_POLLIN, 0}};
      long tm = static_cast<long>(timeout.count());
      auto rc = zmq_poll(poll_item, 1, tm);
      if (rc == -1) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      return rc > 0;
    }

    /**
     * @brief Receive multipart message
     *
     * todo, overflow underflow description here
     *
     * @tparam N
     * @return std::optional<tl::expected<RecvData<N>, std::runtime_error>>
     */
    template <size_t N>
    auto recv_n()
        -> std::optional<tl::expected<RecvData<N>, std::runtime_error>> {
      RecvData<N> data{};

      using zq::currentZmqRuntimeError;

      auto rc = zmq_msg_recv(std::addressof(data.messages[0].msg),
                             socket_ptr.get(), ZMQ_DONTWAIT);

      if (rc == -1) {
        if (zmq_errno() == EAGAIN) {
          return std::nullopt;
        } else {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
      }
      data.msg_count++;

      int more = 0;
      size_t more_size = sizeof(more);
      // read expected num of messages, no more data, return what we have
      for (size_t i = 1; i < N; ++i) {
        rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
        if (rc == -1) {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
        if (!more) {
          return data;
        }
        rc = zmq_msg_recv(std::addressof(data.messages[i].msg),
                          socket_ptr.get(), 0);
        if (rc == -1) {
          return tl::make_unexpected(currentZmqRuntimeError());
        }
        data.msg_count++;
      }
      // one more 'more' check for the overflow case is required
      rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
      if (rc == -1) {
        return tl::make_unexpected(currentZmqRuntimeError());
      }
      if (more) {
        data.msg_count++;
        ;
      }

      return data;
    }
  };

  /**
   * @brief Subscribe a subscriber to a list of topics
   *
   * @param subscriber
   * @param topic_filters
   * @return tl::expected<size_t, std::runtime_error>
   */
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
