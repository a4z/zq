#pragma once

#include <concepts>
#include <ranges>
#include <tl/expected.hpp>

#include "config.hpp"
#include "error.hpp"
#include "socket.hpp"
#include "zflags.hpp"

namespace zq {

  using CtxOptionValue = int;

  struct ContextOption {
    CtxOptionName name;
    CtxOptionValue value;
  };

  template <typename T>
  concept ContextOptions = requires(T container) {
  // clang on github actions does not support this yet, needs XCode 15, has 14
#if __cpp_lib_ranges >= 201911L
    requires std::ranges::range<T>;
#endif
    requires std::same_as<ContextOption, typename T::value_type>;
  };

  constexpr auto DefaultContextOptions = {
      ContextOption{.name = CtxOptionName::IO_THREADS, .value = 1},
      ContextOption{.name = CtxOptionName::IPV6, .value = 0},
      ContextOption{zq::CtxOptionName::BLOCKY, 0},
  };

  class Context {
    // The actual constructor
    friend tl::expected<Context, ZmqError> mk_context(
        ContextOptions auto const& options) noexcept;

    void* z_ctx{nullptr};

    Context(void* ctx) noexcept : z_ctx{ctx} {};

    enum class SocketCon {
      BIND,
      CONNECT,
    };

    /**
     * @brief Internal connector function
     *  Binds or connects to a given endpoint
     * @internal
     * @param con
     * @param type
     * @param endpoint
     * @return tl::expected<Socket, ZmqError>
     */
    [[nodiscard]] tl::expected<Socket, ZmqError> bind_or_connect(
        SocketCon con,
        SocketType type,
        std::string_view endpoint) noexcept {
      auto z_socket = zmq_socket(z_ctx, static_cast<int>(type));
      if (z_socket == nullptr) {
        return tl::make_unexpected(currentZmqError());
      }
      const int err_code = con == SocketCon::BIND
                               ? zmq_bind(z_socket, endpoint.data())
                               : zmq_connect(z_socket, endpoint.data());
      if (err_code != 0) {
        return tl::make_unexpected(currentZmqError());
      }
      // ZMQ_LINGER
      {
        // int zmq_setsockopt (void *socket, int option_name, const void
        // *option_value, size_t option_len);
        int opt_value = 0;
        size_t opt_len = sizeof(opt_value);
        auto rc =
            zmq_setsockopt(z_socket, static_cast<int>(SocketOptionName::LINGER),
                           std::addressof(opt_value), opt_len);
        if (rc != 0) {
          return tl::make_unexpected(currentZmqError());
        }
      }
      SocketPointer sp{z_socket};
      /* Explicitly create the expected objects, otherwise, the return value
       optimization will not be invoked and the Socket's move constructor and
       destructor will be called. */
      return tl::expected<Socket, ZmqError>{std::move(sp)};
    }

   public:
    Context(Context&& rhs) noexcept {
      z_ctx = rhs.z_ctx;
      rhs.z_ctx = nullptr;
    };

    ~Context() noexcept { [[maybe_unused]] auto _ = close(); };

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    /**
     * @brief Close the context
     *
     * Called by the destructor
     * But since closing might block for a while, depending on options
     * its sometimes better to call this explicitly
     *
     * @return ZmqErrorNo code
     */
    [[nodiscard]] ZmqErrorNo close() {
      if (z_ctx != nullptr) {
        if (zmq_ctx_term(z_ctx) != 0) {
          return ZmqErrorNo{zmq_errno()};
        }
      }
      return NoError;
    }

    /**
     * @brief Bind a socket to an endpoint
     *
     * @param type
     * @param endpoint
     * @return tl::expected<Socket, ZmqError>
     */
    [[nodiscard]] tl::expected<Socket, ZmqError> bind(
        SocketType type,
        std::string_view endpoint) noexcept {
      return bind_or_connect(SocketCon::BIND, type, endpoint);
      // validate no publisher ?
    }

    /**
     * @brief Connect a socket to an endpoint
     *
     * @param type
     * @param endpoint
     * @return tl::expected<Socket, ZmqError>
     */
    [[nodiscard]] tl::expected<Socket, ZmqError> connect(
        SocketType type,
        std::string_view endpoint) noexcept {
      return bind_or_connect(SocketCon::CONNECT, type, endpoint);
      // validate no subscriber ?
    }
  };

  /**
   * @brief Factory function for creating a context
   *  This version takes a list of options
   * @param options
   * @return tl::expected<Context, ZmqError>
   */
  [[nodiscard]] auto inline mk_context(
      ContextOptions auto const& options) noexcept
      -> tl::expected<Context, ZmqError> {
    auto z_ctx = zmq_ctx_new();
    if (z_ctx == nullptr) {
      return tl::make_unexpected(currentZmqError());
    }
    Context ctx{z_ctx};
    for (auto const& option : options) {
      if (zmq_ctx_set(z_ctx, static_cast<int>(option.name), option.value) !=
          0) {
        return tl::make_unexpected(currentZmqError());
      }
    }
    return ctx;
  }

  /**
   * @brief Factory function for creating a context
   *  This version uses the default options
   * @return tl::expected<Context, ZmqError>
   */
  [[nodiscard]] auto inline mk_context() noexcept
      -> tl::expected<Context, ZmqError> {
    return mk_context(DefaultContextOptions);
  }

}  // namespace zq
