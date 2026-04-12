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
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
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
# cpp-linenoise: header-only, download only (its CMakeLists.txt needs CMake < 3.5)
FetchContent_Declare(
    cpp_linenoise
    GIT_REPOSITORY https://github.com/yhirose/cpp-linenoise.git
    GIT_TAG master
)
FetchContent_MakeAvailable(httplib doctest)
FetchContent_Populate(cpp_linenoise)

# Shared source files (excluding main and tests)
set(LIB_SOURCES
  src/config/config.cpp
  src/json/json.cpp
  src/ollama/ollama.cpp
  src/repl/repl.cpp
  src/command/command.cpp
  src/annotation/annotation.cpp
  src/exec/exec.cpp
)
set(SRC_INCLUDE src ${cpp_linenoise_SOURCE_DIR})

# Main binary
add_executable(llama-cli src/main.cpp ${LIB_SOURCES})
target_include_directories(llama-cli PRIVATE ${SRC_INCLUDE})
target_link_libraries(llama-cli PRIVATE httplib::httplib)

# Unit tests
add_executable(test_config src/config/config_test.cpp src/config/config.cpp)
target_include_directories(test_config PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_config PRIVATE doctest::doctest)

add_executable(test_json src/json/json_test.cpp src/json/json.cpp)
target_include_directories(test_json PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_json PRIVATE doctest::doctest)

add_executable(test_command src/command/command_test.cpp src/command/command.cpp)
target_include_directories(test_command PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_command PRIVATE doctest::doctest)

add_executable(test_annotation src/annotation/annotation_test.cpp src/annotation/annotation.cpp)
target_include_directories(test_annotation PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_annotation PRIVATE doctest::doctest)

add_executable(test_exec src/exec/exec_test.cpp src/exec/exec.cpp)
target_include_directories(test_exec PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_exec PRIVATE doctest::doctest)

set(REPL_LIBS src/repl/repl.cpp src/command/command.cpp src/annotation/annotation.cpp src/exec/exec.cpp)

add_executable(test_repl src/repl/repl_test.cpp ${REPL_LIBS})
target_include_directories(test_repl PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_repl PRIVATE doctest::doctest)

# Integration tests
add_executable(test_conversation src/repl/conversation_it.cpp ${REPL_LIBS})
target_include_directories(test_conversation PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_conversation PRIVATE doctest::doctest)

add_executable(test_commands src/command/commands_it.cpp ${REPL_LIBS})
target_include_directories(test_commands PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_commands PRIVATE doctest::doctest)

add_executable(test_options src/repl/options_it.cpp ${REPL_LIBS})
target_include_directories(test_options PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_options PRIVATE doctest::doctest)

add_executable(test_annotations src/annotation/annotations_it.cpp ${REPL_LIBS})
target_include_directories(test_annotations PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_annotations PRIVATE doctest::doctest)

add_executable(test_markdown src/tui/markdown_it.cpp ${REPL_LIBS})
target_include_directories(test_markdown PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_markdown PRIVATE doctest::doctest)
