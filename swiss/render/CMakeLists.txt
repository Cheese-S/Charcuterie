cmake_minimum_required(VERSION 4.0)

project(
    swiss_render
    LANGUAGES CXX
)

add_library(swiss_render
    # BEGIN_FILE_GEN src/

	src/Bvh.cpp
	src/Camera.cpp

    # END_GEN
)

target_include_directories(swiss_render PUBLIC
    include
)

target_link_libraries(swiss_render PUBLIC
    milk_core
    milk_asset
)

# BEGIN_TEST_GEN ./test swiss_render


add_test(BvhTest 
         SOURCES test/BvhTest.cpp 
         INCLUDES include interface milk
         LIBS swiss_render milk_test)
# END_GEN

