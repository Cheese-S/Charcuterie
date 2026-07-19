cmake_minimum_required(VERSION 4.0)

project(
    milk_asset
    LANGUAGES CXX
)

add_library(milk_asset
    # BEGIN_FILE_GEN src/import/builder

	src/import/builder/GltfIrBuilder.cpp
	src/import/builder/MeshAssetBuilder.cpp
	src/import/builder/TemplateAssetBuilder.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/export/raw

	src/export/raw/Ppm.cpp

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

# BEGIN_TEST_GEN ./test milk_asset


add_test(PpmTest 
         SOURCES test/export/raw/PpmTest.cpp 
         INCLUDES include interface milk
         LIBS milk_asset milk_test)

add_test(GltfIrBuilderTest 
         SOURCES test/import/builder/GltfIrBuilderTest.cpp 
         INCLUDES include interface milk
         LIBS milk_asset milk_test)
# END_GEN
