
include_directories(${CMAKE_SOURCE_DIR}/src/include)

set(SOURCE_FILES
    test-channel.cc
    test-worker.cc
)

add_executable(thread-pool-test ${SOURCE_FILES})

target_link_libraries(thread-pool-test PRIVATE gtest gtest_main thread-pool)

target_include_directories(thread-pool-test PRIVATE
    ${googletest_SOURCE_DIR}/googletest/include
    ${googletest_SOURCE_DIR}/googlemock/include
)

add_test(NAME TestChannel COMMAND thread-pool-test TestChannel)

add_test(NAME TestWorker COMMAND thread-pool-test TestWorker)