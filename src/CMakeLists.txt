
include_directories(${CMAKE_SOURCE_DIR}/src/include)

if (NOT ENABLE_TEST)
    message(STATUS "Tests are disabled.")
    add_executable(thread-pool main.cc)
endif()