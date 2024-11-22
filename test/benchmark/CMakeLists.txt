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

add_executable(thread-pool-benchmark test-performance.cc)

target_link_libraries(thread-pool-benchmark PRIVATE benchmark::benchmark benchmark::benchmark_main)

target_include_directories(thread-pool-benchmark PRIVATE ${CMAKE_SOURCE_DIR})