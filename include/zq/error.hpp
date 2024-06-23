#pragma once

#include <fmt/core.h>
#include <ostream>
#include "config.hpp"

namespace zq {
  // TODO, if this is the only one that brings in strong_type, review the
  // dependency
  class Error {
   public:
    constexpr explicit Error(int value) : value_(value) {}

    // Ordered
    constexpr bool operator<(const Error& other) const {
      return value_ < other.value_;
    }
    constexpr bool operator>(const Error& other) const {
      return value_ > other.value_;
    }
    constexpr bool operator<=(const Error& other) const {
      return value_ <= other.value_;
    }
    constexpr bool operator>=(const Error& other) const {
      return value_ >= other.value_;
    }
    // Equality
    constexpr bool operator==(const Error& other) const {
      return value_ == other.value_;
    }
    constexpr bool operator!=(const Error& other) const {
      return value_ != other.value_;
    }
    // Comparison with int
    constexpr bool operator<(int other) const { return value_ < other; }
    constexpr bool operator>(int other) const { return value_ > other; }
    constexpr bool operator<=(int other) const { return value_ <= other; }
    constexpr bool operator>=(int other) const { return value_ >= other; }
    constexpr bool operator==(int other) const { return value_ == other; }
    constexpr bool operator!=(int other) const { return value_ != other; }
    // Boolean
    constexpr explicit operator bool() const { return value_ != 0; }
    // Ostreamable
    friend std::ostream& operator<<(std::ostream& os, const Error& error) {
      return os << error.value_;
    }

    constexpr int value() const { return value_; }

   private:
    int value_;
  };

  // backwards compatibility to old Error definition, might be removed one day
  constexpr int value_of(const Error& error) {
    return error.value();
  }

  constexpr Error NoError{0};

  struct ErrMsg {
    Error error;
    std::string message;
  };

  inline Error currentError() noexcept {
    return Error{zmq_errno()};
  }

  inline ErrMsg currentErrMsg() noexcept {
    return ErrMsg{currentError(), zmq_strerror(zmq_errno())};
  }

}  // namespace zq

template <>
struct fmt::formatter<zq::Error> : public fmt::formatter<int> {
  template <typename FormatContext>
  constexpr auto format(const zq::Error& e,
                        FormatContext& ctx) const -> decltype(ctx.out()) {
    return fmt::formatter<int>::format(e.value(), ctx);
  }
};

template <>
struct fmt::formatter<zq::ErrMsg> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const zq::ErrMsg& e, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}, {}", e.error, e.message);
  }
};

template <>
struct fmt::formatter<std::runtime_error> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const std::runtime_error& e, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}", e.what());
  }
};

namespace zq {

  inline auto currentZmqRuntimeError() noexcept {
    return std::runtime_error(fmt::format("{}", currentErrMsg()));
  }

}  // namespace zq
