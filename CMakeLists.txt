cmake_minimum_required(VERSION 3.25)
project(gramina VERSION 0.1.0 LANGUAGES C)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PARSER_SOURCES src/parser/*.c)
file(GLOB_RECURSE COMMON_SOURCES src/common/*.c)
file(GLOB_RECURSE COMPILER_SOURCES src/compiler/*.c)
file(GLOB_RECURSE CLI_SOURCES src/cli/*.c)
file(GLOB_RECURSE TEST_UTIL_SOURCES tests/utils/*.c)
file(GLOB_RECURSE UNIT_TEST_SOURCES tests/unit/**.c)

add_custom_target(GenerateHeaders ALL
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/gen_extra_includes.py
)

include_directories(${LLVM_INCLUDES})
add_compile_options(${LLVM_CFLAGS})
link_directories(${LLVM_LIBDIR} ${LLVM_LDFLAGS})

include_directories("include")
add_compile_definitions(GRAMINA_BUILD)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(GRAMINA_DEBUG_BUILD)
endif ()

add_library(graparse STATIC ${PARSER_SOURCES})
add_library(gracommon STATIC ${COMMON_SOURCES})
add_library(gracompile STATIC ${COMPILER_SOURCES})
add_library(gratestutils STATIC ${TEST_UTIL_SOURCES})

target_include_directories(gratestutils PRIVATE "tests/include")

# TODO: Switch to a more compatible approach
set(LLVM_LIBS "LLVM-19")

add_executable(gramina ${CLI_SOURCES})
add_executable(unit_tests tests/unit.c ${UNIT_TEST_SOURCES})

target_include_directories(unit_tests PRIVATE "tests/include")

# TODO: Find a more compatible approach
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_link_libraries(gramina asan)
    target_link_libraries(unit_tests asan)
    target_compile_options(graparse PRIVATE -fsanitize=address)
    target_compile_options(gracommon PRIVATE -fsanitize=address)
    target_compile_options(gracompile PRIVATE -fsanitize=address)
    target_compile_options(gratestutils PRIVATE -fsanitize=address)
    target_compile_options(gramina PRIVATE -fsanitize=address)
    target_compile_options(unit_tests PRIVATE -fsanitize=address)
endif ()

target_link_libraries(gramina m gracompile graparse gracommon ${LLVM_LIBS})
target_link_libraries(unit_tests m gratestutils gracompile graparse gracommon ${LLVM_LIBS})
