cmake_minimum_required(VERSION 4.0)

project(
    milk_benchmark
    LANGUAGES CXX
)

add_library(milk_benchmark INTERFACE)

target_include_directories(milk_benchmark INTERFACE
    interface
)
