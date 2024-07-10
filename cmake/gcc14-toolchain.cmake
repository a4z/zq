find_program(GCC_PATH gcc-14)
find_program(GPP_PATH g++-14)

if(NOT GCC_PATH)
    message(FATAL_ERROR "${GCC_PATH} not found")
endif()

if(NOT GPP_PATH)
    message(FATAL_ERROR "GPP_PATH not found")
endif()


message("Tripplet: set CMAKE_C_COMPILER " ${GCC_PATH})
message("Tripplet: set CMAKE_CXX_COMPILER " ${GPP_PATH})

SET(CMAKE_C_COMPILER ${GCC_PATH})
SET(CMAKE_CXX_COMPILER ${GPP_PATH})

if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-ld_classic")
endif()
