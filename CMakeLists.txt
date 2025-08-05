
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

# For segmentation fault debugging
option(ENABLE_ADDRESS_SANITIZER "Enable Address Sanitizer" ON)
# For memory leak debugging
option(ENABLE_VALGRIND "Enable Valgrind" OFF)

if(ENABLE_ADDRESS_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -ggdb")
    message(STATUS "Address Sanitizer is enabled")
elseif(ENABLE_VALGRIND)
    message(STATUS "Valgrind is enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
else()
    message(STATUS "Neither Address Sanitizer nor Valgrind is enabled")
endif()


set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/third-party)

include(FetchContent)

include_directories(${CMAKE_SOURCE_DIR}/include)

# gtest
option(ENABLE_BASE_TEST "Enable building base tests using Google Test" ON)
option(ENABLE_BENCHMARK "Enable building benchmarks" OFF)

# link the library atomic for 128 bit atomic operations
find_library(LIBATOMIC_LIBRARY
    atomic
    HINTS
    /usr/lib/aarch64-linux-gnu
    /usr/lib/gcc/aarch64-linux-gnu/13
)
if(LIBATOMIC_LIBRARY)
    message(STATUS "Found libatomic: ${LIBATOMIC_LIBRARY}")
    add_definitions(-DUSE_LIBATOMIC)
    link_libraries(${LIBATOMIC_LIBRARY})
else()
    message(STATUS "libatomic not found. Using internal implementation.")
endif()


# add the executable
if(ENABLE_BASE_TEST)
    message(STATUS "Base tests are enabled. Adding Google Test.")
    add_definitions(-DENABLE_BASE_TEST)
    enable_testing()
    add_subdirectory(test/base-test)
elseif(ENABLE_BENCHMARK)
    message(STATUS "Benchmarks are enabled.")
    add_definitions(-DENABLE_BENCHMARK)
    add_subdirectory(test/benchmark)
else()
    message(STATUS "Tests are disabled.")
endif()
