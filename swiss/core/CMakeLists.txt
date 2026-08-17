cmake_minimum_required(VERSION 4.0)

project(
    swiss_core
    LANGUAGES CXX
)

add_library(swiss_core
    # BEGIN_FILE_GEN src/

	src/PathTracer.cpp

    # END_GEN
)

target_link_libraries(swiss_core PUBLIC
    swiss_render
    milk_core
    milk_asset
)

target_include_directories(swiss_core PUBLIC
    include
)

