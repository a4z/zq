#pragma once

#include <cstring>
#include <iostream>
#include <limits>
#include "a4z/typename.hpp"
#include "config.hpp"
#include "error.hpp"

namespace zq {

  // std::string as a typename does not always work,
  // might be std::string, or class std::basic_string<char,struct
  // std::char_traits<char>,class std::allocator<char> >> depending on the
  // platform, therefore a typename that fits into SSO is choosen
  static inline const auto str_type_name = "zq::str";

  /**
   * @brief Wrapper for a zmq_msg_t message
   *
   * A RAII interface. Plus some convenience functions.
   */
  struct Message {
    zmq_msg_t msg;

    Message() noexcept { zmq_msg_init(std::addressof(msg)); }

    explicit Message(size_t size) noexcept {
      zmq_msg_init_size(std::addressof(msg), size);
    }

    // avoid copying
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    Message(Message&& rhs) noexcept {
      zmq_msg_init(std::addressof(msg));
      zmq_msg_move(std::addressof(msg), std::addressof(rhs.msg));
    }

    Message& operator=(Message&& rhs) noexcept {
      // this only happens with an invalid message, basically never
      // but for debugging purposes we should check it
      [[maybe_unused]] auto r = zmq_msg_close(std::addressof(msg));
      if (r != 0) {
        if constexpr (debug_build) {
          auto ce = currentZmqError();
          std::cerr << "zmq_msg_close: " << ce.errNo << ", " << ce.what()
                    << std::endl;
        }
      }
      zmq_msg_init(std::addressof(msg));
      zmq_msg_move(std::addressof(msg), std::addressof(rhs.msg));
      return *this;
    }
    ~Message() noexcept {
      // this only happens with an invalid message, basically never
      // but for debugging purposes we should check it
      [[maybe_unused]] auto r = zmq_msg_close(std::addressof(msg));
      if (r != 0) {
        if constexpr (debug_build) {
          auto ce = currentZmqError();
          std::cerr << "zmq_msg_close: " << ce.errNo << ", " << ce.what()
                    << std::endl;
        }
      }
    }

    std::size_t size() const noexcept {
      return zmq_msg_size(std::addressof(msg));
    }

    void* data() noexcept { return zmq_msg_data(std::addressof(msg)); }

    const void* data() const noexcept {
      return zmq_msg_data(const_cast<zmq_msg_t*>(std::addressof(msg)));
    }
  };

  /**
   * @brief Aultipart message, composed from the type and a payload
   *
   * The type is a string (message), the payload is the serialized data.
   * By sending this to the other side, the other side can restore the
   * payload to the original type
   *
   */
  struct TypedMessage {
    Message type;
    Message payload;

    TypedMessage() noexcept = default;

    TypedMessage(Message t, Message p) noexcept
        : type(std::move(t)), payload(std::move(p)) {}

    TypedMessage(TypedMessage&&) noexcept = default;
    TypedMessage& operator=(TypedMessage&&) noexcept = default;

    TypedMessage(const TypedMessage&) = delete;
    TypedMessage& operator=(const TypedMessage&) = delete;
    ~TypedMessage() noexcept = default;
  };

  // create / restore typed messages

  // char array shall convert to a string view
  template <typename T>
  concept is_char_array =
      std::is_array_v<T> &&
      std::is_same_v<std::remove_cv_t<std::remove_extent_t<T>>, char>;

  template <typename T>
  concept mem_copyable = std::is_trivially_copyable_v<T>;

  template <typename T>
  concept mem_copyable_message = mem_copyable<T> && !is_char_array<T>;

  template <typename T>
  concept is_message = std::is_same_v<T, Message>;

  template <typename... Args>
  concept pack_of_messages = (is_message<Args> && ...);

  // create type name message
  template <typename T>
  Message typename_message() {
    constexpr auto tn = a4z::type_name<T>();
    Message m(tn.size());
    memcpy(m.data(), tn.data, tn.size());
    return m;
  }

  inline Message str_message(std::string_view val) {
    Message m(val.size());
    memcpy(m.data(), val.data(), val.size());
    return m;
  }

  inline TypedMessage typed_message(std::string_view str) {
    Message payload{str.size()};
    memcpy(payload.data(), str.data(), str.size());
    return TypedMessage(str_message(str_type_name), std::move(payload));
  }

  template <typename T>
  inline TypedMessage typed_message(const T& value)
    requires mem_copyable_message<T>
  {
    Message payload{sizeof(T)};
    std::memcpy(payload.data(), std::addressof(value), sizeof(T));
    return TypedMessage(typename_message<T>(), std::move(payload));
  }

  // in case I know it's a string, like the type part of a typed message
  // guess those could be constexpr ... TODO
  inline std::string as_string(const Message& m) {
    const char* data = reinterpret_cast<const char*>(m.data());
    return std::string{data, m.size()};
  }

  inline std::string_view as_string_view(const Message& m) {
    const char* data = reinterpret_cast<const char*>(m.data());
    return std::string_view{data, m.size()};
  }

  // restore typed messages

  template <typename T>
  using restore_result = tl::expected<T, ZqError>;

  // This is the default template function for restore_as.
  // Will trigger a static assert for unsupported types
  // Users can specialize this template for their own types
  template <typename T>
  auto restore_as(const TypedMessage&) noexcept -> restore_result<T> {
    static_assert(!std::is_same_v<T, T>, "Unsupported type");
  }

  // This is a template specialization for restore_as function for std::string.
  template <>
  inline auto restore_as<std::string>(const TypedMessage& msg) noexcept
      -> restore_result<std::string> {
    const auto having_name = as_string_view(msg.type);
    if (having_name != str_type_name) {
      return tl::make_unexpected(ZqError("Message type does not match"));
    }
    return as_string(msg.payload);
  }

  // This is a template specialization for restore_as function for types that
  // are mem_copyable_message.
  template <typename T>
    requires mem_copyable_message<T>
  inline auto restore_as(const TypedMessage& msg) noexcept
      -> restore_result<T> {
    auto check_type_name = [&]() {
      constexpr auto tn = a4z::type_name<T>();
      if (msg.type.size() != tn.size()) {
        return false;
      }
      const auto expected_name = std::string_view(tn.c_str(), tn.size());
      const auto having_name = as_string_view(msg.type);
      return expected_name == having_name;
    };

    auto unexpected = [](std::string_view err_msg) {
      return tl::make_unexpected(ZqError(err_msg.data()));
    };

    if (!check_type_name()) {
      return unexpected("Message type does not match");
    }
    if (msg.payload.size() != sizeof(T)) {
      return unexpected("Data size does not match");
    }
    T value;
    memcpy(std::addressof(value), msg.payload.data(), sizeof(T));
    return value;
  }

}  // namespace zq
