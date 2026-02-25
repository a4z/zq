#pragma once

#include <zmq.h>
#include <expected>

#ifdef NDEBUG
constexpr bool debug_build = false;
#else
constexpr bool debug_build = true;
#endif
