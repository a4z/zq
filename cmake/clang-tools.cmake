add_custom_target(
  clang-tidy
  COMMAND git ls-files | grep -E "^(include|src)/.*\.(c|cpp|h|hpp)$" | xargs clang-tidy --extra-arg=-std=c++${CMAKE_CXX_STANDARD} -p ${CMAKE_BINARY_DIR} --header-filter='^$'
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM
  USES_TERMINAL
)

add_custom_target(
  clang_tidy
  COMMAND ${CMAKE_SOURCE_DIR}/run_clang_tidy.sh ${CMAKE_CXX_STANDARD} ${CMAKE_BINARY_DIR}
  COMMENT "Running clang-tidy on each file individually"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
)

if(WIN32)
  set(FORMAT_COMMAND powershell -Command "git ls-files | Where-Object {$_ -match '.*\\.(c|h|cpp|hpp)$'} | ForEach-Object {clang-format -i $_}")
  set(FORMAT_CHECK_COMMAND powershell -Command "git ls-files | Where-Object {$_ -match '.*\\.(c|h|cpp|hpp)$'} | ForEach-Object {clang-format --Werror --dry-run $_}")
else()
  set(FORMAT_COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format -i)
  set(FORMAT_CHECK_COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format --Werror --dry-run)
endif()

add_custom_target(
  clang-format
  # COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format -i
  COMMAND ${FORMAT_COMMAND}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM
)

add_custom_target(
  clang-format-check
  # COMMAND git ls-files | grep -E ".*\\.(c|h|cpp|hpp)$" | xargs clang-format --Werror --dry-run
  COMMAND ${FORMAT_CHECK_COMMAND}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM
  USES_TERMINAL
)
