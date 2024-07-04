#pragma once

#include <string>
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
    std::string as_string() const {
      std::string msg = std::to_string(error.value()) + ": " + message;
      return msg;
    }
    friend std::ostream& operator<<(std::ostream& os, const ErrMsg& msg) {
      return os << msg.as_string();
    }
  };

  inline Error currentError() noexcept {
    return Error{zmq_errno()};
  }

  inline ErrMsg currentErrMsg() noexcept {
    return ErrMsg{currentError(), zmq_strerror(zmq_errno())};
  }

  inline auto currentZmqRuntimeError() noexcept {
    const auto ce = currentErrMsg();
    return std::runtime_error(ce.as_string());
  }

}  // namespace zq

