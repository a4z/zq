cmake_minimum_required(VERSION 3.27)

project(zq VERSION 0.1.0
        LANGUAGES CXX
        DESCRIPTION "A specialised and opioionated C++ wrapper around ZeroMQ")

if(PROJECT_IS_TOP_LEVEL)
    # make git ignore the build directory
    file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")
    option(ZQ_TEST_PROJECT "Enable testing" ON) # give option to test even if we are not the top project
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_custom_target(
        clangformat
        COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format -i
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
endif()

option(ZQ_WITH_PROTO "Build with protobuf tests" ON)

set(CMAKE_CXX_STANDARD 20)

find_package(astr CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED) # until apple clang catches up


list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (${ZQ_TEST_PROJECT})
    include(CTest)
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/message_proto.hpp> # doesnt matter to have the header if it's not used
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/socket.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/zflags.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/zq/zq.hpp>
)
target_include_directories(zq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


if (ZQ_WITH_PROTO)
    find_package(Protobuf CONFIG REQUIRED)
    target_compile_definitions(zq INTERFACE "ZQ_PROTO")
    set(Protobuf_DEPENDENCY "find_dependency(Protobuf)")
    add_subdirectory(proto)
endif()

target_link_libraries(zq
    INTERFACE
      libzmq
      tl::expected # until C++23
      fmt::fmt # until apple clang catches up
      a4z::astr
)

if (ZQ_WITH_PROTO)
    target_link_libraries(zq
        INTERFACE
        protobuf::libprotobuf
    )
endif()

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


