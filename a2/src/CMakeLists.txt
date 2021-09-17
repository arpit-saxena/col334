include_directories(${PROJECT_SOURCE_DIR}/include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(COMMON_FILES client_socket.cpp message.cpp)
add_library(common STATIC ${COMMON_FILES})
add_executable(client client.cpp)
target_link_libraries(client PUBLIC common pthread)
add_executable(server server.cpp)
target_link_libraries(server PUBLIC common pthread)

set_target_properties(client server
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
