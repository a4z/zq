cmake_minimum_required(VERSION 3.27)

project(zq VERSION 0.1.0
        LANGUAGES CXX
        DESCRIPTION "A specialised and opioionated C++ wrapper around ZeroMQ")

if(PROJECT_IS_TOP_LEVEL)
    # make git ignore the build directory
    file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")
    set(DO_TEST_PROJECT ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_custom_target(
        clangformat
        COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format -i
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
endif()

set(CMAKE_CXX_STANDARD 20)

# Prepare for consumption of astr via vcpkg
option(FETCH_ASTR "Use fetch content to get a4z::astr" ON)
if (FETCH_ASTR)
    message(STATUS "Fetching ASTR")
    include(FetchContent)
    FetchContent_Declare(
    astr
    GIT_REPOSITORY https://github.com/a4z/astr.git
    GIT_TAG        main
    OVERRIDE_FIND_PACKAGE
    )
    set(astrEXPORT "astr") # not optimal, but for now
    # basicallly it should be so, if I fetch something,
    # the make install stuff should not be part of the game ....
endif()

find_package(astr CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED) # until apple clang catches up
find_package(Protobuf CONFIG REQUIRED)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(setup_compiler)
include(default_flags)

option (BUILD_TESTING "Build the testing tree." ${DO_TEST_PROJECT})
enable_testing()
if (BUILD_TESTING)
    include(testing)
    add_subdirectory(tests)
endif()

add_subdirectory(src)

add_library(zq INTERFACE)
add_library(a4z::zq ALIAS zq)
target_sources(zq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/config.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/context.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/error.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/message.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/socket.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/zflags.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/zq.hpp>
)
target_include_directories(zq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_subdirectory(proto)


target_link_libraries(zq
    INTERFACE
      libzmq
      tl::expected # until C++23
      fmt::fmt # until apple clang catches up
      protobuf::libprotobuf
      a4z::astr
)

include(GNUInstallDirs)
install(TARGETS zq ${astrEXPORT} EXPORT zqTargets INCLUDES DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT zqTargets
    FILE zqTargets.cmake
    NAMESPACE a4z::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/zqConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/zqConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zqConfig.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/zqConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/zqConfigVersion.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)


