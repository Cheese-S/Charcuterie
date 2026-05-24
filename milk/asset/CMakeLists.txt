cmake_minimum_required(VERSION 4.0)

project(
    milk_asset
    LANGUAGES CXX
)

add_library(milk_asset
    # BEGIN_FILE_GEN src/import/raw

	src/import/raw/Gltf.cpp

    # END_GEN
)

target_include_directories(milk_asset PUBLIC
    interface
)

target_include_directories(milk_asset PRIVATE
    include
)

target_link_libraries(milk_asset PUBLIC
    milk_core
)

# link libs
target_link_libraries(milk_asset PRIVATE 
    cgltf
)
