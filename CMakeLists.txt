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

include(FetchContent)

FetchContent_Declare(
  astr
  GIT_REPOSITORY https://github.com/a4z/astr.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(astr)

option(COVERAGE "Enable coverage reporting" OFF)

if(COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(COVERAGE_BRANCHES "--rc lcov_branch_coverage=0")
    separate_arguments(COVERAGE_BRANCHES)
    set(COVERAGE_WARNINGS "--ignore-errors gcov")
    if(APPLE)
        set(COVERAGE_WARNINGS "--ignore-errors gcov --ignore-errors inconsistent --ignore-errors unused")
    endif()
    separate_arguments(COVERAGE_WARNINGS)
    add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND lcov  --directory . --capture --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info ${COVERAGE_WARNINGS} ${COVERAGE_BRANCHES}
    COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info '/usr/*' '*/tests/*' '*/vcpkg_installed/*' '${CMAKE_SOURCE_DIR}/external/*' --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info.cleaned ${COVERAGE_WARNINGS} ${COVERAGE_BRANCHES}
    COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage/coverage.info.cleaned --output-directory ${CMAKE_BINARY_DIR}/coverage  ${COVERAGE_BRANCHES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

endif()

message(STATUS "*********Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "*********CMAKE_CXX_COMPILER file: ${CMAKE_CXX_COMPILER}")


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
      strong_type::strong_type
      fmt::fmt # until apple clang catches up
      protobuf::libprotobuf
      a4z::astr

)



