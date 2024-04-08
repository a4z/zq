
get_filename_component(TRIPLET_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
include(${TRIPLET_DIR}/gcc-13-toolchain.cmake)
include($ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake)

