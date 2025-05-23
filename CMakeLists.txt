cmake_minimum_required(VERSION 3.25)
project(gramina VERSION 0.1.0 LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PARSER_SOURCES src/parser/*.c)
file(GLOB_RECURSE COMMON_SOURCES src/common/*.c)
file(GLOB_RECURSE COMPILER_SOURCES src/compiler/*.c)

add_custom_target(GenerateHeaders ALL
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/gen_extra_includes.py
)

include_directories(${LLVM_INCLUDES})
add_compile_options(${LLVM_CFLAGS})
link_directories(${LLVM_LIBDIR} ${LLVM_LDFLAGS})

include_directories("include")
add_compile_definitions(GRAMINA_BUILD)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(GRAMINA_DEBUG_BUILD)
endif()

add_library(graparse STATIC ${PARSER_SOURCES})
add_library(gracommon STATIC ${COMMON_SOURCES})
add_library(gracompile STATIC ${COMPILER_SOURCES})

# TODO: Switch to a more compatible approach
set(LLVM_LIBS "LLVM-19")

add_executable(test src/test.c)

# TODO: Find a more compatible approach
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(test asan)
    target_compile_options(graparse PRIVATE -fsanitize=address)
    target_compile_options(gracommon PRIVATE -fsanitize=address)
    target_compile_options(gracompile PRIVATE -fsanitize=address)
    target_compile_options(test PRIVATE -fsanitize=address)
endif()

target_link_libraries(test m graparse gracompile gracommon ${LLVM_LIBS})
