
if(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_INSTALLATION_ROOT})
elseif(DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
else()
  message(FATAL_ERROR "Neither VCPKG_INSTALLATION_ROOT nor VCPKG_ROOT environment variables are defined.")
endif()

set(_overlay "${CMAKE_SOURCE_DIR}/cmake/overlay-ports")
if(EXISTS "${_overlay}")
  list(APPEND VCPKG_OVERLAY_PORTS "${_overlay}")
endif()
unset(_overlay)

include(${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

