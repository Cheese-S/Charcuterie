cmake_minimum_required(VERSION 4.0)

project(
    milk_test
    LANGUAGES CXX
)

add_library(milk_test INTERFACE)

target_include_directories(milk_test INTERFACE
    interface
)


