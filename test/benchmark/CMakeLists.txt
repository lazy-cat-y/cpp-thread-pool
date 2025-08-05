FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        main 
  SOURCE_DIR     ${FETCHCONTENT_BASE_DIR}/googletest
  BINARY_DIR     ${FETCHCONTENT_BASE_DIR}/googletest-build
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG main
)
FetchContent_MakeAvailable(benchmark)

option(BENCHMARK_ATOMIC_QUEUE "Enable benchmark for atomic queue" ON)

if (BENCHMARK_ATOMIC_QUEUE)
  add_executable(benchmark-test test-atomic-queue.cc)
else()
add_executable(thread-pool-benchmark test-performance.cc)
endif()

target_link_libraries(benchmark-test PRIVATE benchmark::benchmark benchmark::benchmark_main atomic)

target_include_directories(benchmark-test PRIVATE ${CMAKE_SOURCE_DIR})