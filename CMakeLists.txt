cmake_minimum_required(VERSION 3.14)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
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
    GIT_TAG v2.5.1
)
# cpp-linenoise: header-only, download only (its CMakeLists.txt needs CMake < 3.5)
FetchContent_Declare(
    cpp_linenoise
    GIT_REPOSITORY https://github.com/yhirose/cpp-linenoise.git
    GIT_TAG master
)
FetchContent_Declare(
    dtl
    GIT_REPOSITORY https://github.com/cubicdaiya/dtl.git
    GIT_TAG v1.20
)
FetchContent_MakeAvailable(httplib doctest)
FetchContent_Populate(cpp_linenoise)
FetchContent_Populate(dtl)

# Shared source files (excluding main and tests)
set(LIB_SOURCES
  src/config/config.cpp
  src/json/json.cpp
  src/ollama/ollama.cpp
  src/repl/repl.cpp
  src/repl/repl_commands.cpp
  src/repl/repl_annotations.cpp
  src/repl/repl_chat.cpp
  src/repl/repl_search.cpp
  src/repl/repl_model.cpp
  src/tui/markdown.cpp
  src/tui/table.cpp
  src/tui/spinner.cpp
  src/command/command.cpp
  src/annotation/annotation.cpp
  src/exec/exec.cpp
  src/exec/hardware.cpp
  src/net/scan.cpp
  src/trace/trace.cpp
  src/logging/logger.cpp
  src/session/session.cpp
  src/sync/sync.cpp
  src/sync/sync_annotations.cpp
  src/util/util.cpp
)
set(SRC_INCLUDE src ${cpp_linenoise_SOURCE_DIR} ${dtl_SOURCE_DIR} src/trace src/logging)

# Get version from git tags (e.g. v0.19.0 → 0.19.0)
execute_process(
  COMMAND git describe --tags --abbrev=0
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE LLAMA_CLI_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
# Strip leading 'v' if present
string(REGEX REPLACE "^v" "" LLAMA_CLI_VERSION "${LLAMA_CLI_VERSION}")
if(LLAMA_CLI_VERSION STREQUAL "")
  set(LLAMA_CLI_VERSION "0.0.0-dev")
endif()
add_compile_definitions(LLAMA_CLI_VERSION="${LLAMA_CLI_VERSION}")

# Inject build timezone at compile time (portable CMake-only)
string(TIMESTAMP BUILD_TZ "%z")
if(BUILD_TZ STREQUAL "")
  set(BUILD_TZ "+0000")
endif()
add_compile_definitions(BUILD_TIMEZONE="${BUILD_TZ}")

# Inject git commit SHA at compile time for session logging
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(GIT_COMMIT STREQUAL "")
  set(GIT_COMMIT "unknown")
endif()
add_compile_definitions(GIT_COMMIT="${GIT_COMMIT}")

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

add_executable(test_hardware src/exec/hardware_test.cpp src/exec/hardware.cpp)
target_include_directories(test_hardware PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_hardware PRIVATE doctest::doctest)

add_executable(test_ollama src/ollama/ollama_test.cpp src/ollama/ollama.cpp src/json/json.cpp src/logging/logger.cpp src/trace/trace.cpp)
target_include_directories(test_ollama PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_ollama PRIVATE doctest::doctest httplib::httplib)

add_executable(test_scan src/net/scan_test.cpp src/net/scan.cpp)
target_include_directories(test_scan PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_scan PRIVATE doctest::doctest httplib::httplib)

add_executable(test_session src/session/session_test.cpp src/session/session.cpp src/json/json.cpp)
target_include_directories(test_session PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_session PRIVATE doctest::doctest)

add_executable(test_sync src/sync/sync_test.cpp src/sync/sync.cpp)
target_include_directories(test_sync PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_sync PRIVATE doctest::doctest)

add_executable(test_util src/util/util_test.cpp src/util/util.cpp)
target_include_directories(test_util PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_util PRIVATE doctest::doctest)

set(REPL_LIBS src/repl/repl.cpp src/repl/repl_commands.cpp src/repl/repl_annotations.cpp src/repl/repl_chat.cpp src/repl/repl_search.cpp src/repl/repl_model.cpp src/tui/markdown.cpp src/tui/table.cpp src/tui/spinner.cpp src/command/command.cpp src/annotation/annotation.cpp src/exec/exec.cpp src/exec/hardware.cpp src/net/scan.cpp src/trace/trace.cpp src/logging/logger.cpp src/ollama/ollama.cpp src/json/json.cpp src/session/session.cpp src/sync/sync.cpp src/sync/sync_annotations.cpp src/util/util.cpp)

add_executable(test_repl src/repl/repl_test.cpp ${REPL_LIBS})
target_include_directories(test_repl PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_repl PRIVATE doctest::doctest httplib::httplib)

# Integration tests
add_executable(test_conversation src/repl/conversation_it.cpp ${REPL_LIBS})
target_include_directories(test_conversation PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_conversation PRIVATE doctest::doctest httplib::httplib)

add_executable(test_commands src/command/commands_it.cpp ${REPL_LIBS})
target_include_directories(test_commands PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_commands PRIVATE doctest::doctest httplib::httplib)

add_executable(test_options src/repl/options_it.cpp ${REPL_LIBS})
target_include_directories(test_options PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_options PRIVATE doctest::doctest httplib::httplib)

add_executable(test_config_it src/config/config_it.cpp src/config/config_test_helper.cpp)
target_include_directories(test_config_it PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_config_it PRIVATE doctest::doctest)

add_executable(test_annotations src/annotation/annotations_it.cpp ${REPL_LIBS})
target_include_directories(test_annotations PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_annotations PRIVATE doctest::doctest httplib::httplib)

add_executable(test_markdown src/tui/markdown_it.cpp ${REPL_LIBS})
target_include_directories(test_markdown PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_markdown PRIVATE doctest::doctest httplib::httplib)

add_executable(test_trace src/trace/trace_test.cpp src/trace/trace.cpp)
target_include_directories(test_trace PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_trace PRIVATE doctest::doctest)

add_executable(test_logger src/logging/logger_test.cpp src/logging/logger.cpp)
target_include_directories(test_logger PRIVATE ${SRC_INCLUDE})
target_link_libraries(test_logger PRIVATE doctest::doctest)

# Fuzz targets — build with -DENABLE_FUZZ=ON (requires clang with libFuzzer)
option(ENABLE_FUZZ "Build fuzz targets (requires clang with -fsanitize=fuzzer)" OFF)
if(ENABLE_FUZZ)
  add_executable(fuzz_annotation src/annotation/fuzz_annotation.cpp src/annotation/annotation.cpp)
  target_include_directories(fuzz_annotation PRIVATE ${SRC_INCLUDE})
  target_compile_options(fuzz_annotation PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(fuzz_annotation PRIVATE -fsanitize=fuzzer,address,undefined)
endif()