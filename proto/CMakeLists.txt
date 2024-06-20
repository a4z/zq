# Get the filenames of all the proto files.
file(GLOB PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

add_library(zqproto STATIC)

MESSAGE(STATUS "Found protoc: ${Protobuf_PROTOC_EXECUTABLE}")

# Generate header and source files for all the proto files
foreach(proto ${PROTO_FILES})
    # Get filename without extension
    get_filename_component(PROTO_NAME_WE ${proto} NAME_WE)
    get_filename_component(PROTO_NAME ${proto} NAME)

    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${proto})

    target_include_directories(zqproto SYSTEM PUBLIC ${Protobuf_INCLUDE_DIRS})
    target_include_directories(zqproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(zqproto PRIVATE ${PROTO_SRCS} ${PROTO_HDRS})

endforeach()
