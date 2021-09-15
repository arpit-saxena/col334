include_directories(${PROJECT_SOURCE_DIR}/include)

set(COMMON_FILES client_socket.cpp message.cpp)
add_library(common STATIC ${COMMON_FILES})
add_executable(client client.cpp)
target_link_libraries(client common)
add_executable(server server.cpp)
target_link_libraries(server common)

set_target_properties(client server
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
