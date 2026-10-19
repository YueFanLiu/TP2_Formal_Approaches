cmake_minimum_required(VERSION 3.16)
project(TP2ParserAST LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(FLEX REQUIRED)
flex_target(Scanner src/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)

add_executable(compiler
    src/main.cpp
    src/parser.cpp
    src/ast.cpp
    ${FLEX_Scanner_OUTPUTS}
)
target_include_directories(compiler PRIVATE include ${FLEX_INCLUDE_DIRS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME parser_regression COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parser.py $<TARGET_FILE:compiler>)
    endif()
endif()
