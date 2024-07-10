#pragma once

#include <ostream>
#include <string>
#include <variant>
#include "config.hpp"

namespace zq {

  /**
   * @brief Encapsulates a ZeroMQ error number.
   *
   *  This class is a simple wrapper around an int, representing a ZeroMQ error
   */
  class ZmqErrorNo {
   public:
    constexpr explicit ZmqErrorNo(int value) : _value(value) {}

    // Ordered
    constexpr bool operator<(const ZmqErrorNo& other) const {
      return _value < other._value;
    }
    constexpr bool operator>(const ZmqErrorNo& other) const {
      return _value > other._value;
    }
    constexpr bool operator<=(const ZmqErrorNo& other) const {
      return _value <= other._value;
    }
    constexpr bool operator>=(const ZmqErrorNo& other) const {
      return _value >= other._value;
    }
    // Equality
    constexpr bool operator==(const ZmqErrorNo& other) const {
      return _value == other._value;
    }
    constexpr bool operator!=(const ZmqErrorNo& other) const {
      return _value != other._value;
    }
    // Comparison with int
    constexpr bool operator<(int other) const { return _value < other; }
    constexpr bool operator>(int other) const { return _value > other; }
    constexpr bool operator<=(int other) const { return _value <= other; }
    constexpr bool operator>=(int other) const { return _value >= other; }
    constexpr bool operator==(int other) const { return _value == other; }
    constexpr bool operator!=(int other) const { return _value != other; }
    // Boolean
    constexpr explicit operator bool() const { return _value != 0; }
    // Ostreamable
    friend std::ostream& operator<<(std::ostream& os, const ZmqErrorNo& error) {
      return os << error._value;
    }

    constexpr int value() const { return _value; }

   private:
    int _value;
  };

  [[deprecated("Use ZmqErrorNo::value() instead.")]] constexpr int value_of(
      const ZmqErrorNo& error) {
    return error.value();
  }

  /**
   * @brief Constant representing no error.
   *
   */
  constexpr ZmqErrorNo NoError{0};

  /**
   * @brief Class encapsulating a ZeroMQ error.
   *
   * If a ZeroMQ function returns an error,
   * this class is used to return the ZeroMQ internal
   * error number and the error message.
   *
   * Note that zq does not throw exceptions, but returns expected values.
   *
   */
  struct ZmqError : public std::runtime_error {
    ZmqErrorNo errNo;

    ZmqError(ZmqErrorNo en, std::string_view msg)
        : std::runtime_error(std::to_string(en.value()) + std::string{": "} +
                             msg.data()),
          errNo(en) {}
  };

  /**
   * @brief Class encapsulating a zq error.
   *
   * This class is used to inform the user about unexpected behavior in zq,
   * or wrong api usage.
   *
   * Note that zq does not throw exceptions, but returns expected values.
   */
  struct ZqError : public std::logic_error {
    using std::logic_error::logic_error;
  };

  /**
   * @brief Error holding either a ZmqError or a ZqError.
   *
   *  Some functions can return both ZmqError and ZqError as the unexpected
   * value
   *
   *  In this case, this Error is used to hold the actual Error type.
   */
  struct Error : public std::exception {
    std::variant<ZmqError, ZqError> error;

    Error(const ZmqError& e) : error(e) {}
    Error(const ZqError& e) : error(e) {}

    bool isZqError() { return std::holds_alternative<ZqError>(error); }

    bool isZmqError() { return std::holds_alternative<ZmqError>(error); }

    const char* what() const noexcept override {
      if (std::holds_alternative<ZmqError>(error)) {
        return std::get<ZmqError>(error).what();
      } else {
        return std::get<ZqError>(error).what();
      }
    }
  };

  inline ZmqErrorNo currentError() noexcept {
    return ZmqErrorNo{zmq_errno()};
  }

  inline ZmqError currentZmqError() noexcept {
    return ZmqError{currentError(), zmq_strerror(zmq_errno())};
  }

}  // namespace zq
