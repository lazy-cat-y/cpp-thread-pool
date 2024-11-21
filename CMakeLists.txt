
cmake_minimum_required(VERSION 3.22)

project(thread-pool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -pthread -O0")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third-party)

include(FetchContent)

add_subdirectory(src)
# gtest
option(ENABLE_TEST "Enable building tests using Google Test" ON)

# add the executable

if (ENABLE_TEST)
    message(STATUS "Tests are enabled. Adding Google Test.")
    enable_testing()
    add_subdirectory(test)
else()
    message(STATUS "Tests are disabled.")
endif()
