cmake_minimum_required(VERSION 4.0)

project(
    benchmark
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(benchmark_cc 
    src/benchmark.cpp
)

target_link_libraries(benchmark_cc PUBLIC
    benchmark::benchmark
    GTest::gtest_main
    GTest::gmock
)


target_compile_options(benchmark_cc PUBLIC
    ${cc_compile_options}
)


