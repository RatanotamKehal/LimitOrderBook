cmake_minimum_required(VERSION 3.20)
project(LimitOrderBook VERSION 1.0 LANGUAGES CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable aggressive warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Include directories
include_directories(include)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Fetch Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_RE2 OFF CACHE BOOL "Disable RE2 dependency" FORCE)
FetchContent_MakeAvailable(benchmark)

# --- Core Engine Library ---
add_library(lob_core src/lob.cpp) # You will need to create src/lob.cpp
target_include_directories(lob_core PUBLIC include)

# --- Testing Target ---
# Uncomment and update once you write tests/test_lob.cpp
# add_executable(lob_tests tests/test_lob.cpp)
# target_link_libraries(lob_tests PRIVATE lob_core gtest_main)
# include(GoogleTest)
# gtest_discover_tests(lob_tests)

# --- Benchmark Target ---
# Uncomment and update once you write benchmarks/benchmark_lob.cpp
# add_executable(lob_benchmark benchmarks/benchmark_lob.cpp)
# target_link_libraries(lob_benchmark PRIVATE lob_core benchmark::benchmark benchmark::benchmark_main)