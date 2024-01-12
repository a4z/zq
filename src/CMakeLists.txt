

# target_link_libraries(main PRIVATE libzmq libzmq-static)

add_executable(example_server example_c_server.cpp)
add_executable(example_client example_c_client.cpp)

target_link_libraries(example_client
  PRIVATE
  libzmq-static
)
target_link_libraries(example_server
  PRIVATE
  libzmq-static
)
