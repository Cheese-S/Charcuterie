cmake_minimum_required(VERSION 4.0)

project(
    swiss_render
    LANGUAGES CXX
)

add_library(swiss_render
    # BEGIN_FILE_GEN src/render

	src/render/Bvh.cpp
	src/render/Camera.cpp

    # END_GEN
)

target_include_directories(swiss_render PUBLIC
    include
)

target_link_libraries(swiss_render PUBLIC
    milk_core
    milk_asset
)

# BEGIN_BENCHMARK_GEN ./benchmark swiss_render


add_benchmark(BvhBenchmark 
         SOURCES benchmark/BvhBenchmark.cpp 
         INCLUDES include interface milk
         LIBS swiss_render milk_benchmark)

# END_GEN

# BEGIN_TEST_GEN ./test swiss_render


add_test(BvhTest 
         SOURCES test/BvhTest.cpp 
         INCLUDES include interface milk
         LIBS swiss_render milk_test)

# END_GEN



