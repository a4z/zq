cmake_minimum_required(VERSION 3.27)

project(zq CXX)

if(PROJECT_IS_TOP_LEVEL)
    # make git ignore the build directory
    file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")

    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

    set(DO_TEST_PROJECT ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
endif()

find_package(astr CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(strong_type CONFIG REQUIRED)
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
target_sources(zq INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/zq/zq.hpp
)
target_include_directories(zq INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_subdirectory(proto)


target_link_libraries(zq
    INTERFACE
      default::flags
      zqproto
      libzmq
      tl::expected
      fmt::fmt # until apple clang catches up
      protobuf::libprotobuf
      a4z::astr

)


file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp include/*.h)
add_custom_target(
    clangformat
    COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format -i
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
