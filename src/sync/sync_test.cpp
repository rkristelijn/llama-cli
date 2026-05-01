/**
 * @file sync_test.cpp
 * @brief Unit tests for sync-mode helper functions.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "sync/sync.h"

#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <string>

// --- has_cap -----------------------------------------------------------------

TEST_CASE ("has_cap finds capability in list") {
  CHECK (has_cap("read,write,exec", "read"))
    ;
  CHECK (has_cap("read,write,exec", "write"))
    ;
  CHECK (has_cap("read,write,exec", "exec"))
    ;
}

TEST_CASE ("has_cap rejects missing capability") {
  CHECK_FALSE (has_cap("read,write", "exec"))
    ;
  CHECK_FALSE (has_cap("", "read"))
    ;
}

TEST_CASE ("has_cap rejects partial matches") {
  CHECK_FALSE (has_cap("readonly", "read"))
    ;
  CHECK_FALSE (has_cap("read", "readwrite"))
    ;
}

TEST_CASE ("has_cap works with single capability") {
  CHECK (has_cap("read", "read"))
    ;
  CHECK_FALSE (has_cap("write", "read"))
    ;
}

// --- is_read_only ------------------------------------------------------------

TEST_CASE ("is_read_only allows safe commands") {
  CHECK (is_read_only("cat src/main.cpp"))
    ;
  CHECK (is_read_only("ls -la"))
    ;
  CHECK (is_read_only("grep -r TODO src/"))
    ;
  CHECK (is_read_only("head -20 file.txt"))
    ;
  CHECK (is_read_only("wc -l src/*.cpp"))
    ;
}

TEST_CASE ("is_read_only allows piped safe commands") {
  CHECK (is_read_only("cat file.txt | grep TODO"))
    ;
  CHECK (is_read_only("find . -name '*.cpp' | sort | head -5"))
    ;
}

TEST_CASE ("is_read_only blocks unsafe commands") {
  CHECK_FALSE (is_read_only("rm -rf /"))
    ;
  CHECK_FALSE (is_read_only("mv a b"))
    ;
  CHECK_FALSE (is_read_only("python script.py"))
    ;
}

TEST_CASE ("is_read_only blocks redirects") {
  CHECK_FALSE (is_read_only("cat file > output.txt"))
    ;
  CHECK_FALSE (is_read_only("echo hi >> log.txt"))
    ;
}

TEST_CASE ("is_read_only blocks unsafe in pipe chain") {
  CHECK_FALSE (is_read_only("cat file.txt | rm -rf /"))
    ;
  CHECK_FALSE (is_read_only("grep TODO | python -c 'import os'"))
    ;
}

// --- parse_exec_tags ---------------------------------------------------------

TEST_CASE ("parse_exec_tags extracts commands") {
  auto cmds = parse_exec_tags("run <exec>ls -la</exec> now");
  REQUIRE (cmds.size() == 1)
    ;
  CHECK (cmds[0] == "ls -la")
    ;
}

TEST_CASE ("parse_exec_tags handles multiple tags") {
  auto cmds = parse_exec_tags("<exec>cmd1</exec> text <exec>cmd2</exec>");
  REQUIRE (cmds.size() == 2)
    ;
  CHECK (cmds[0] == "cmd1")
    ;
  CHECK (cmds[1] == "cmd2")
    ;
}

TEST_CASE ("parse_exec_tags returns empty for no tags") {
  CHECK (parse_exec_tags("no tags here").empty())
    ;
  CHECK (parse_exec_tags("").empty())
    ;
}

TEST_CASE ("parse_exec_tags handles unclosed tag") {
  auto cmds = parse_exec_tags("<exec>unclosed");
  CHECK (cmds.empty())
    ;
}

// --- path_allowed ------------------------------------------------------------

TEST_CASE ("path_allowed allows files inside sandbox") {
  CHECK (path_allowed("/tmp", "/tmp"))
    ;
  // Create a temp file to test with
  std::ofstream("/tmp/llama-test-sandbox.txt") << "test";
  CHECK (path_allowed("/tmp/llama-test-sandbox.txt", "/tmp"))
    ;
  std::remove("/tmp/llama-test-sandbox.txt");
}

TEST_CASE ("path_allowed blocks files outside sandbox") {
  CHECK_FALSE (path_allowed("/etc/passwd", "/tmp"))
    ;
  CHECK_FALSE (path_allowed("/tmp/../etc/passwd", "/tmp"))
    ;
}

TEST_CASE ("path_allowed handles nonexistent sandbox") {
  CHECK_FALSE (path_allowed("/tmp/file.txt", "/nonexistent/sandbox"))
    ;
}

TEST_CASE ("path_allowed allows new files in sandbox") {
  // File doesn't exist but parent dir is in sandbox
  CHECK (path_allowed("/tmp/llama-new-file-test.txt", "/tmp"))
    ;
}

// --- extract_resources -------------------------------------------------------

TEST_CASE ("extract_resources finds file:// paths") {
  std::string json = R"({"resources":["file://src/main.cpp","file://README.md"]})";
  auto paths = extract_resources(json);
  REQUIRE (paths.size() == 2)
    ;
  CHECK (paths[0] == "src/main.cpp")
    ;
  CHECK (paths[1] == "README.md")
    ;
}

TEST_CASE ("extract_resources returns empty for no resources") {
  CHECK (extract_resources("{}").empty())
    ;
  CHECK (extract_resources("").empty())
    ;
}

TEST_CASE ("extract_resources handles single resource") {
  auto paths = extract_resources(R"(["file://test.txt"])");
  REQUIRE (paths.size() == 1)
    ;
  CHECK (paths[0] == "test.txt")
    ;
}
