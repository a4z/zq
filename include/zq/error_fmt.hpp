#pragma once

#include <fmt/core.h>
#include <ostream>
#include "error.hpp"


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
