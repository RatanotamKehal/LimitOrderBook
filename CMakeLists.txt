cmake_minimum_required(VERSION 3.24)
project(HighPerfLOB VERSION 1.0 LANGUAGES CXX)

# Enforce C++20 and strict warnings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common compiler flags for hardware sympathy and strictness
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Fetch Google Benchmark
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/v1.8.3.tar.gz
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Core Library
add_library(lob_core src/lob.cpp)
target_include_directories(lob_core PUBLIC include)

# Tests
add_executable(lob_tests tests/test_lob.cpp)
target_link_libraries(lob_tests PRIVATE lob_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(lob_tests)

# Benchmarks
add_executable(lob_benchmark benchmarks/bench_lob.cpp)
target_link_libraries(lob_benchmark PRIVATE lob_core benchmark::benchmark)