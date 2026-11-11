cmake_minimum_required(VERSION 3.16)
project(pasc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Werror)

# Runtime library linked into every compiled Pascal program. It is never
# built with sanitizers, because the programs that link it are not.
add_library(pasrt STATIC runtime/rt.c)

add_executable(pasc
  src/ast_dump.cpp
  src/codegen_x86.cpp
  src/diag.cpp
  src/lexer.cpp
  src/main.cpp
  src/parser.cpp
  src/sema.cpp
  src/types.cpp)
target_compile_definitions(pasc PRIVATE PASC_RT_DEFAULT="$<TARGET_FILE:pasrt>")
add_dependencies(pasc pasrt)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(pasc PRIVATE -fsanitize=address,undefined)
  target_link_options(pasc PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME conformance
         COMMAND ${CMAKE_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:pasc>)
