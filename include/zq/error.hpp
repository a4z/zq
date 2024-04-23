#pragma once

#include <strong_type/strong_type.hpp>

#include "config.hpp"

namespace zq
{
  // TODO, if this is the only one that brings in strong_type, review the dependency
  using Error = strong::type<int,
                               struct zq_error_tag,
                               strong::ordered,
                               strong::ordered_with<int>,
                               strong::equality,
                               strong::equality_with<int>,
                               strong::boolean,
                               strong::ostreamable>;

  constexpr Error NoError{0};

  struct ErrMsg {
    Error error;
    std::string message;
  };

  inline Error currentError() noexcept {
    return Error{ zmq_errno() };
  }

  inline ErrMsg currentErrMsg() noexcept {
    return ErrMsg{ currentError(), zmq_strerror(zmq_errno()) };
  }


} // namespace zq

template <>
struct fmt::formatter<zq::ErrMsg> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const zq::ErrMsg& e, fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}, {}", e.error, e.message);
    }
};

namespace zq
{

  inline auto currentZmqRuntimeError() noexcept {
      return std::runtime_error(fmt::format("{}", currentErrMsg()));
  }

} // namespace zq

template <>
struct fmt::formatter<std::runtime_error> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const std::runtime_error& e, fmt::format_context& ctx) {
        return fmt::format_to(ctx.out(), "{}", e.what());
    }
};
