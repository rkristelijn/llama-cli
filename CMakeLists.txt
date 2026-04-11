cmake_minimum_required(VERSION 3.14)
project(llama-cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ccache — speed up rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Sanitizers (ASan + UBSan) — enable with -DENABLE_SANITIZERS=ON
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Warnings as errors
add_compile_options(-Wall -Wextra -Werror)

include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.7
)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_Declare(
    cpp_linenoise
    GIT_REPOSITORY https://github.com/yhirose/cpp-linenoise.git
    GIT_TAG master
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_MakeAvailable(httplib doctest)
FetchContent_GetProperties(cpp_linenoise)
if(NOT cpp_linenoise_POPULATED)
  FetchContent_Populate(cpp_linenoise)
endif()

# Shared source files (excluding main)
set(LIB_SOURCES src/config.cpp src/json.cpp src/ollama.cpp src/repl.cpp src/command.cpp src/annotation.cpp src/exec.cpp)

# Main binary
add_executable(llama-cli src/main.cpp ${LIB_SOURCES})
target_include_directories(llama-cli PRIVATE include ${cpp_linenoise_SOURCE_DIR})
target_link_libraries(llama-cli PRIVATE httplib::httplib)

# Config unit tests
add_executable(test_config test/test_config.cpp src/config.cpp)
target_include_directories(test_config PRIVATE include)
target_link_libraries(test_config PRIVATE doctest::doctest)

# REPL unit tests
add_executable(test_repl test/test_repl.cpp src/repl.cpp src/command.cpp src/annotation.cpp src/exec.cpp)
target_include_directories(test_repl PRIVATE include ${cpp_linenoise_SOURCE_DIR})
target_link_libraries(test_repl PRIVATE doctest::doctest)

# Command unit tests
add_executable(test_command test/test_command.cpp src/command.cpp)
target_include_directories(test_command PRIVATE include)
target_link_libraries(test_command PRIVATE doctest::doctest)

# Annotation unit tests
add_executable(test_annotation test/test_annotation.cpp src/annotation.cpp)
target_include_directories(test_annotation PRIVATE include)
target_link_libraries(test_annotation PRIVATE doctest::doctest)

# JSON unit tests
add_executable(test_json test/test_json.cpp src/json.cpp)
target_include_directories(test_json PRIVATE include)
target_link_libraries(test_json PRIVATE doctest::doctest)

# Exec unit tests
add_executable(test_exec test/test_exec.cpp src/exec.cpp)
target_include_directories(test_exec PRIVATE include)
target_link_libraries(test_exec PRIVATE doctest::doctest)
