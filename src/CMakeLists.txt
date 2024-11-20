
include_directories(${CMAKE_SOURCE_DIR}/src/include)

set(SOURCE_FILES
    main.cc
)

add_library(thread-pool ${SOURCE_FILES})

if (NOT ENABLE_TEST)
    message(STATUS "Tests are disabled.")
    add_executable(thread-pool main.cc)
endif()