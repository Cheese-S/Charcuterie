cmake_minimum_required(VERSION 4.0)

project(
    milk_render   
    LANGUAGES CXX
)

add_library(milk_render
    # BEGIN_FILE_GEN src

	src/RhiVk.cpp

    # END_GEN
)

target_include_directories(milk_render PUBLIC
    interface
)

target_include_directories(milk_render PUBLIC
    include
)

# link libs
target_link_libraries(milk_render PUBLIC volk)
