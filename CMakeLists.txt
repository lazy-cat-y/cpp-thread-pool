
cmake_minimum_required(VERSION 3.22)

project(thread-pool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -pthread -O1")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third-party)

include(FetchContent)

message(STATUS "Fetching Google Test.")
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        main 
  SOURCE_DIR     ${FETCHCONTENT_BASE_DIR}/googletest
  BINARY_DIR     ${FETCHCONTENT_BASE_DIR}/googletest-build
)

FetchContent_MakeAvailable(googletest)

FetchContent_GetProperties(googletest)
if (googletest_POPULATED)
    message(STATUS "Google Test is populated successfully.")
else()
    message(FATAL_ERROR "Google Test population failed.")
    FetchContent_Populate(googletest)
endif()

message(STATUS "Google Test source dir: ${googletest_SOURCE_DIR}")
message(STATUS "Google Test binary dir: ${googletest_BINARY_DIR}")

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
