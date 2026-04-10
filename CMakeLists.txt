cmake_minimum_required(VERSION 3.14)
project(llama-cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.7
)
FetchContent_MakeAvailable(httplib)

# Shared source files (excluding main)
set(LIB_SOURCES src/config.cpp src/json.cpp src/ollama.cpp)

# Main binary
add_executable(llama-cli src/main.cpp ${LIB_SOURCES})
target_include_directories(llama-cli PRIVATE include)
target_link_libraries(llama-cli PRIVATE httplib::httplib)

# Config unit tests
add_executable(test_config test/test_config.cpp src/config.cpp)
target_include_directories(test_config PRIVATE include)

# JSON unit tests
add_executable(test_json test/test_json.cpp src/json.cpp)
target_include_directories(test_json PRIVATE include)
