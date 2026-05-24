cmake_minimum_required(VERSION 4.0)

project(
    swiss
    LANGUAGES CXX
)

add_executable(swiss
    main.cpp

    # BEGIN_FILE_GEN src/

	src/PathTracer.cpp

    # END_GEN
)

target_include_directories(swiss PRIVATE
    include
)

target_link_libraries(swiss PRIVATE
    milk_core       
)
