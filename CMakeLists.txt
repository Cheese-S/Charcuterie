cmake_minimum_required(VERSION 4.0)

project(
    Charcuterie
    LANGUAGES CXX
)

# ------------------------------------ PROJECT WIDE ----------------------------
set(CMAKE_LINK_LIBRARIES_STRATEGY REORDER_MINIMALLY)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINKER_TYPE LLD)

set(CMAKE_CXX_COMPILER_LAUNCHER "ccache")
set(CMAKE_C_COMPILER_LAUNCHER "ccache")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(CC_ASAN "Compile with ASAN" OFF)
option(CC_DEBUG "Compile with Debug mode" OFF)
option(CC_CLANG_CL "check if generating for CLANG-CL" OFF)

if (CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES "MSVC") 
    set(CC_CLANG_CL ON)
endif()

if (CMAKE_BUILD_TYPE MATCHES "[dD]ebug") 
    set(CC_DEBUG ON)
endif()

if (CC_CLANG_CL)
    set(cc_compile_options
        /W4
        /WX
        -Wno-deprecated
        -Wpedantic
        "/clang:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/="
    )

    set(cc_link_options
        /INCREMENTAL
    )
else()
    set(cc_compile_options 
        -Wall 
        -Werror
        -Wextra
        -Wpedantic
        -Wno-deprecated
    )
endif()


if (CC_DEBUG)
    list(APPEND cc_compile_options -g)
endif()


function (add_test name) 
    set(argPattern SOURCES INCLUDES LIBS)

    cmake_parse_arguments(ARG "" "" "${argPattern}" ${ARGN})

    add_executable(
        ${name}
        ${ARG_SOURCES}
    )   
    target_compile_options(${name} PUBLIC ${cc_compile_options})
    target_link_libraries(${name} PUBLIC
        GTest::gtest_main
        GTest::gmock
        ${ARG_LIBS})
    target_include_directories(${name} PUBLIC ${ARG_INCLUDES})
    message(STATUS "add_test ${name} ${source} ${ARG_SOURCES} ${ARG_INCLUDES} ${ARG_LIBS}")
endfunction()


if(NOT EXISTS "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/codemodel-v2")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/.cmake/api/v1/query")
    file(TOUCH "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/codemodel-v2")
endif()

#if (CC_ASAN)
    # list(APPEND cc_compile_options -fsanitize=address)
    # list(APPEND cc_link_options -fsanitize=address)
#endif()

add_subdirectory(third_party)

# test
add_subdirectory(milk/test)

# libs
add_subdirectory(milk/core)
add_subdirectory(milk/render)
add_subdirectory(milk/asset)

# apps
add_subdirectory(swiss)

# --------------------------- APPS --------------------------


add_executable(cc
    cc/main.cpp
)

target_link_libraries(cc PUBLIC milk_core)

message(STATUS "compile options: ${cc_compile_options}")

# -------------------------- MINJECT PATCHING ----------------------
set(MIMALLOC_POSTFIX_LIST)
#
# if (CC_ASAN)
#     list(APPEND MIMALLOC_POSTFIX "asan")
# endif()
#
if (CC_DEBUG) 
    list(APPEND MIMALLOC_POSTFIX_LIST debug)
endif()

list(JOIN MIMALLOC_POSTFIX_LIST "-"  MIMALLOC_POSTFIX)
message(STATUS "Mimalloc postfix: ${MIMALLOC_POSTFIX}")

# --------------------------- TESTS ------------------------------

function (minjectPatching target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/third_party/mimalloc/bin/minject.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        # COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/minject.exe -i --postfix=${MIMALLOC_POSTFIX} -f $<TARGET_FILE:${target}>
        # COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/minject.exe -l $<TARGET_FILE:${target}>
    )
endfunction()

# -------------------------- BENCHMARK --------------------
add_subdirectory(benchmark)
