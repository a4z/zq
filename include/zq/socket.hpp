#pragma once

#include "a4z/typename.hpp"
#include "config.hpp"
#include "message.hpp"
#include "zflags.hpp"

#include <array>
#include <chrono>
#include <memory>
#include <string_view>
#include <vector>

namespace zq {

  /**
   * @brief Deleter for the unique_ptr
   *
   * Since socket resets the pointer, this is never called in zq code
   * but to be technically correct, this deleter is needed for the unique_ptr
   *
   */
  struct ZmqSocketClose {
    void operator()(void* ptr) const {
      if (ptr) {
        zmq_close(ptr);
      }
    }
  };

  /// Concept to check if a type is std::array of Message
  template <typename T>
  concept StdArrayOfMessage = requires(T t) {
    requires std::is_same_v<
        decltype(t), std::array<Message, std::tuple_size<decltype(t)>::value>>;
  };

  /// Concept declaring a Message Container
  template <typename T>
  concept MessageContainer =
      std::is_same_v<T, std::vector<Message>> || StdArrayOfMessage<T>;
  ;

  /**
   * @brief Indicates the result of a receive operation
   *
   *  If you read a multipart message, you can get an underflow or overflow.
   *  Underflow means that not the expected number of messages was received.
   *  Overflow means that more messages were received than expected.
   *
   */
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

  /// @brief  Shortcut for a unique_ptr to a zmq socket
  using SocketPointer = std::unique_ptr<void, ZmqSocketClose>;

  /**
   * @brief ZMQ Socket wrapper
   *
   *  Manages a socket and provides some convenience functions for sending
   *  and receiving messages. In especially, TypeMessage, but also other forms
   *  of multipart messages.
   *  And a single message can also be sent, of course
   */
  struct Socket {
    SocketPointer socket_ptr{nullptr};

    /// @brief Construct a socket from a pointer
    Socket(SocketPointer socket) noexcept : socket_ptr{std::move(socket)} {};
    /// @brief Move constructor
    Socket(Socket&& rhs) noexcept : socket_ptr{std::move(rhs.socket_ptr)} {}

    Socket(Socket const&) = delete;
    Socket& operator=(Socket const&) = delete;
    Socket& operator=(Socket&&) = delete;

    /// @brief Destructor, ensures the socket is closed
    ~Socket() noexcept { [[maybe_unused]] auto _ = close(); }

    /**
     * @brief Close the socket
     *
     * @return Error code
     */
    [[nodiscard]] ZmqErrorNo close() noexcept {
      if (socket_ptr != nullptr) {
        if (zmq_close(socket_ptr.get()) != 0) {
          return ZmqErrorNo{zmq_errno()};
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
     * @return std::expected<size_t, ZmqError> Number of bytes sent, or an error
     */
    template <pack_of_messages... Messages>
    [[nodiscard]] std::expected<size_t, ZmqError> send(
        const Message& first,
        const Messages&... messages) {
      size_t bytes_sent = 0;

      if constexpr (sizeof...(messages) == 0) {
        auto rc = zmq_send(socket_ptr.get(), first.data(), first.size(),
                           ZMQ_DONTWAIT);
        if (rc < 0) {
          return std::unexpected(currentZmqError());
        }
        bytes_sent += static_cast<size_t>(rc);
      } else {
        auto rc =
            zmq_send(socket_ptr.get(), first.data(), first.size(), ZMQ_SNDMORE);
        if (rc < 0) {
          return std::unexpected(currentZmqError());
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
     * @return std::expected<size_t, ZmqError>
     */
    [[nodiscard]] std::expected<size_t, ZmqError> send(
        const TypedMessage& msg) {
      return send(msg.type, msg.payload);
    }

    /**
     * @brief Send a vector or array of Message elements
     *
     * @tparam Container (std::vector<Message> or std::array<Message, N>)
     * @param msg
     * @return std::expected<size_t, ZmqError>
     */
    template <MessageContainer Container>
    [[nodiscard]] std::expected<size_t, ZmqError> send(const Container& msg) {
      size_t bytes_sent = 0;
      size_t total_messages = msg.size();
      for (size_t i = 0; i < total_messages; ++i) {
        // Determine the flag based on whether this is the last message
        int flags = (i == total_messages - 1) ? ZMQ_DONTWAIT : ZMQ_SNDMORE;
        auto& m = msg[i];
        auto rc = zmq_send(socket_ptr.get(), m.data(), m.size(), flags);
        if (rc < 0) {
          return std::unexpected(currentZmqError());
        }
        bytes_sent += static_cast<size_t>(rc);
      }
      return bytes_sent;
    }

    /**
     * @brief Return a typed message
     *
     *  Returns an Error with a ZmqError if there was a ZMQ issue
     *  Returns an Error with a ZqError if there was no multipart message
     *  but only a single one.
     *
     * \note re rec_n or rec_all message are more resilient in
     * case of messages that have more then two parts. In case it's not 100%
     * sure TypeMessage is communicated, better use one of them.
     *
     * @return std::optional<std::expected<TypedMessage, Error>>
     */
    [[nodiscard]] std::optional<std::expected<TypedMessage, Error>> recv() {
      TypedMessage typed_message;

      auto rc = zmq_msg_recv(std::addressof(typed_message.type.msg),
                             socket_ptr.get(), ZMQ_DONTWAIT);

      if (rc == -1) {
        if (zmq_errno() == EAGAIN) {
          return std::nullopt;
        } else {
          return std::unexpected(currentZmqError());
        }
      }
      int more = 0;
      size_t more_size = sizeof(more);
      rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
      if (rc == -1) {
        return std::unexpected(currentZmqError());
      }
      if (more) {
        rc = zmq_msg_recv(std::addressof(typed_message.payload.msg),
                          socket_ptr.get(), 0);
        if (rc == -1) {
          return std::unexpected(currentZmqError());
        }
      } else {
        return std::unexpected(ZqError("no more message"));
      }

      return typed_message;
    }

    /**
     * @brief Receive as many message parts as there are on the socket
     *
     * @return std::optional<std::expected<Message, ZmqError>>
     */
    [[nodiscard]] std::optional<std::expected<std::vector<Message>, ZmqError>>
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
            return std::unexpected(currentZmqError());
          }
        }
        rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
        if (rc == -1) {
          return std::unexpected(currentZmqError());
        }
      }
      return messages;
    }

    /**
     * @brief await a TypedMessage for the given timeout
     *
     * Wait for the given timeout, if a message is available,
     * calls recv and returns the result.
     *
     * If no message is available, returns nullopt
     *
     * @param timeout
     * @return std::optional<std::expected<TypedMessage, Error>>
     */
    [[nodiscard]] std::optional<std::expected<TypedMessage, Error>> await(
        std::chrono::milliseconds timeout) {
      zmq_pollitem_t poll_item[] = {{socket_ptr.get(), 0, ZMQ_POLLIN, 0}};
      long tm = static_cast<long>(timeout.count());
      auto rc = zmq_poll(poll_item, 1, tm);
      if (rc == -1) {
        return std::unexpected(currentZmqError());
      }

      return recv();
    }

    /**
     * @brief Poll for given timeout
     *
     * Returns true if there is data to receive, false otherwise
     *
     * @param timeout
     * @return std::expected<bool, ZmqError>
     */
    [[nodiscard]] std::expected<bool, ZmqError> poll(
        std::chrono::milliseconds timeout) {
      zmq_pollitem_t poll_item[] = {{socket_ptr.get(), 0, ZMQ_POLLIN, 0}};
      long tm = static_cast<long>(timeout.count());
      auto rc = zmq_poll(poll_item, 1, tm);
      if (rc == -1) {
        return std::unexpected(currentZmqError());
      }
      return rc > 0;
    }

    /**
     * @brief Receive multipart message
     *
     * todo, overflow underflow description here
     *
     * @tparam N
     * @return std::optional<std::expected<RecvData<N>, ZmqError>>
     */
    template <size_t N>
    auto recv_n() -> std::optional<std::expected<RecvData<N>, ZmqError>> {
      RecvData<N> data{};

      auto rc = zmq_msg_recv(std::addressof(data.messages[0].msg),
                             socket_ptr.get(), ZMQ_DONTWAIT);

      if (rc == -1) {
        if (zmq_errno() == EAGAIN) {
          return std::nullopt;
        } else {
          return std::unexpected(currentZmqError());
        }
      }
      data.msg_count++;

      int more = 0;
      size_t more_size = sizeof(more);
      // read expected num of messages, no more data, return what we have
      for (size_t i = 1; i < N; ++i) {
        rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
        if (rc == -1) {
          return std::unexpected(currentZmqError());
        }
        if (!more) {
          return data;
        }
        rc = zmq_msg_recv(std::addressof(data.messages[i].msg),
                          socket_ptr.get(), 0);
        if (rc == -1) {
          return std::unexpected(currentZmqError());
        }
        data.msg_count++;
      }
      // one more 'more' check for the overflow case is required
      rc = zmq_getsockopt(socket_ptr.get(), ZMQ_RCVMORE, &more, &more_size);
      if (rc == -1) {
        return std::unexpected(currentZmqError());
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
   * In case of an error, a ZqError is returned if the given socket is not a
   * subscriber otherwise, in case of a Zmq error, a ZmqError is returned.
   *
   * @param subscriber
   * @param topic_filters
   * @return std::expected<size_t, Error>
   */
  [[nodiscard]] inline std::expected<size_t, Error> subscribe(
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
        return std::unexpected(currentZmqError());
      }
      if (val != ZMQ_SUB) {
        return std::unexpected(ZqError("socket is not a subscriber"));
      }
    }

    if (topic_filters.size() == 0) {
      auto rc =
          zmq_setsockopt(subscriber.socket_ptr.get(), ZMQ_SUBSCRIBE, "", 0);
      if (rc != 0) {
        return std::unexpected(currentZmqError());
      }
      return std::numeric_limits<size_t>::max();
    }
    // subscribe to requested topics
    size_t topicCount = 0;
    for (auto&& t : topic_filters) {
      auto rc = zmq_setsockopt(subscriber.socket_ptr.get(), ZMQ_SUBSCRIBE,
                               t.data(), t.size());
      if (rc != 0) {
        return std::unexpected(currentZmqError());
      }
      topicCount++;
    }
    return topicCount;
  }

}  // namespace zq
