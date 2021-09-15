include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(client client.cpp client_socket.cpp)
add_executable(server server.cpp client_socket.cpp)

set_target_properties(client server
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
