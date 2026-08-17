cmake_minimum_required(VERSION 4.0)

project(
    swiss
    LANGUAGES CXX
)

add_subdirectory(render)
add_subdirectory(core)

add_executable(swiss
    main.cpp
)

target_link_libraries(swiss PRIVATE
    swiss_core
    swiss_render
    milk_core
)
