
message(STATUS "Fetching Google Test.")
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
    SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/googletest
    BINARY_DIR ${FETCHCONTENT_BASE_DIR}/googletest-build
)

FetchContent_MakeAvailable(googletest)

FetchContent_GetProperties(googletest)
if(googletest_POPULATED)
    message(STATUS "Google Test is populated successfully.")
else()
    message(FATAL_ERROR "Google Test population failed.")
    FetchContent_Populate(googletest)
endif()

message(STATUS "Google Test source dir: ${googletest_SOURCE_DIR}")
message(STATUS "Google Test binary dir: ${googletest_BINARY_DIR}")

set(SOURCE_FILES
    mpmc_queue_test.cc
    thread_pool_test.cc
)

add_executable(thread-pool-test ${SOURCE_FILES})

target_link_libraries(thread-pool-test PRIVATE gtest gtest_main)

target_include_directories(thread-pool-test PRIVATE
    ${googletest_SOURCE_DIR}/googletest/include
    ${googletest_SOURCE_DIR}/googlemock/include
)


add_test(NAME MPMCQueueTest COMMAND thread-pool-test MPMCQueueTest)

add_test(NAME ThreadPoolTest COMMAND thread-pool-test ThreadPoolTest)
