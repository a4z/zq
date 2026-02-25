include_guard(GLOBAL)


find_package(doctest CONFIG REQUIRED)

add_library(doctest_main STATIC ${PROJECT_SOURCE_DIR}/tests/test_main.cpp)

set(TEST_FRAMEWORK doctest::doctest)

target_link_libraries(doctest_main PUBLIC ${TEST_FRAMEWORK})

function (add_doctest NAME)

    set(option_args WILL_FAIL)
    set(value_args TIMEOUT)
    set(list_args SOURCES)

    cmake_parse_arguments(D_TEST
        "${option_args}" "${value_args}" "${list_args}"
        ${ARGN}
    )

    ##add_executable(${NAME} ${D_TEST_SOURCES} $<TARGET_OBJECTS:doctest_main>)
    add_executable(${NAME} ${D_TEST_SOURCES})
    target_link_libraries(${NAME} doctest_main)

    target_link_libraries(${NAME} ${TEST_FRAMEWORK} zq a4z::commonCompilerWarnings)
    if (ZQ_WITH_PROTO)
        target_link_libraries(${NAME} zqproto)
    endif()

    if(NOT D_TEST_TIMEOUT)
        set(D_TEST_TIMEOUT 3)
    endif()
    if(NOT D_TEST_WILL_FAIL)
        set(D_TEST_WILL_FAIL OFF)
    endif()

    add_test(NAME ${NAME} COMMAND ${NAME})

    set_tests_properties(${NAME}
        PROPERTIES
            TIMEOUT ${D_TEST_TIMEOUT}
            WILL_FAIL ${D_TEST_WILL_FAIL}
    )

endfunction(add_doctest)




