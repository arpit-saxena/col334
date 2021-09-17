cmake_minimum_required(VERSION 3.15)

project(
    ChatApp
    VERSION 0.1
    DESCRIPTION "A chat application that allows users to exchange plain text messages with each other"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

configure_file(${PROJECT_SOURCE_DIR}/include/config.hpp.in ${PROJECT_SOURCE_DIR}/include/config.hpp)
add_subdirectory(src)