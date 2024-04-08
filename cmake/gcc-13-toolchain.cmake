# IF (APPLE)
#      mabye add as hint to find program?
#     /opt/homebrew/bin
# endif(APPLE)

find_program (GCC13 "gcc-13")
find_program (GPP13 "g++-13")

MESSAGE(STATUS "Found gcc-13:" ${GCC13})
MESSAGE(STATUS "Found g++-13:" ${GPP13})

SET(CMAKE_C_COMPILER ${GCC13})
SET(CMAKE_CXX_COMPILER ${GPP13})



