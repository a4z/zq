find_program(GCC_PATH gcc-13)
find_program(GPP_PATH g++-13)

if(NOT GCC_PATH)
    message(FATAL_ERROR "gcc-13 not found")
endif()

if(NOT GPP_PATH)
    message(FATAL_ERROR "g++-13 not found")
endif()

SET(CMAKE_C_COMPILER ${GCC_PATH})
SET(CMAKE_CXX_COMPILER ${GPP_PATH})

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-ld_classic")
endif()
