cmake_minimum_required(VERSION 4.0)

project(
    third_party
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------- MIMALLOC -----------------------
set(MI_BUILD_STATIC OFF CACHE BOOL "Build static library" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "Build object library" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "Build test executables" FORCE)
# We only build the shared libarry. On windows, that's the only way to intercept other
# libraries' calls
add_subdirectory(mimalloc)

# ---------------------- FMT -----------------------
add_subdirectory(fmt)

# ---------------------- VOLK -------------------
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
add_subdirectory(volk)

# ---------------------- MCMP Queue -------------------
add_library(MCMPQueue INTERFACE)
target_include_directories(MCMPQueue INTERFACE ./MCMPQueue/)


# ---------------------- BENCHMARK -------------------
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(benchmark)

# ---------------------- GTEST ----------------------
add_subdirectory(gtest)

# ---------------------- GLM ------------------------
add_subdirectory(glm)

# ---------------------- CGLTF ----------------------
add_library(cgltf INTERFACE)
target_include_directories(cgltf SYSTEM INTERFACE ./cgltf/)

