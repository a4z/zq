

# target_link_libraries(main PRIVATE libzmq libzmq-static)

add_executable(example_c example_c.cpp)

target_link_libraries(example_c
  PRIVATE
  libzmq-static
)
