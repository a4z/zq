#pragma once


#include <zmq.h>
#include <fmt/format.h>
#include <tl/expected.hpp>

#ifdef NDEBUG
constexpr bool debug_build = false;
#else
constexpr bool debug_build = true;
#endif
