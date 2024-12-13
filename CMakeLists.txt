
cmake_minimum_required(VERSION 3.22)

project(thread-pool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -pthread -O0")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -O3")
endif()

set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third-party)

include(FetchContent)

include_directories(${CMAKE_SOURCE_DIR}/include)

# gtest
option(ENABLE_BASE_TEST "Enable building base tests using Google Test" ON)
option(ENABLE_BENCHMARK "Enable building benchmarks" OFF)

# add the executable
if (ENABLE_BASE_TEST)
    message(STATUS "Base tests are enabled. Adding Google Test.")
    add_definitions(-DENABLE_TEST)
    enable_testing()
    add_subdirectory(test/base-test)
elseif(ENABLE_BENCHMARK)
    message(STATUS "Benchmarks are enabled.")
    add_subdirectory(test/benchmark)
else()
    message(STATUS "Tests are disabled.")
endif()
