cmake_minimum_required(VERSION 3.24)
project(HighPerfLOB VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

include(FetchContent)

# --- Google Test ---
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Google Benchmark ---
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/v1.9.1.tar.gz
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(HAVE_STD_REGEX ON CACHE BOOL "Bypass regex check" FORCE)
FetchContent_MakeAvailable(benchmark)

# --- Compiler Flags ---
set(LOB_COMPILER_FLAGS
  $<$<CXX_COMPILER_ID:GNU,Clang>: -Wall -Wextra -Werror -Wpedantic -march=native>
  $<$<CXX_COMPILER_ID:MSVC>: /W4 /WX>
  $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -DNDEBUG>>
  $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-O0 -g>>
)

# --- Initial (Naive) Library ---
add_library(lob_initial src/initial/lob.cpp)
target_include_directories(lob_initial PUBLIC include)
target_compile_options(lob_initial PRIVATE ${LOB_COMPILER_FLAGS})

# --- Optimized Library ---
add_library(lob_optimized src/optimized/lob.cpp)
target_include_directories(lob_optimized PUBLIC include)
target_compile_options(lob_optimized PRIVATE ${LOB_COMPILER_FLAGS})

# --- Tests ---
add_executable(lob_tests tests/tests_lob.cpp)
target_link_libraries(lob_tests PRIVATE lob_initial lob_optimized gtest_main)

include(GoogleTest)
gtest_discover_tests(lob_tests)

# --- Benchmarks ---
add_executable(lob_benchmark benchmarks/bench_lob.cpp)
target_link_libraries(lob_benchmark PRIVATE 
  lob_initial 
  lob_optimized 
  benchmark::benchmark 
  Threads::Threads
)