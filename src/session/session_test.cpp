/**
 * @file session_test.cpp
 * @brief Unit tests for session persistence (load/save).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "session/session.h"

#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <string>

TEST_CASE ("load_session returns empty for nonexistent file") {
  auto msgs = load_session("/tmp/llama-test-no-such-file.json");
  CHECK (msgs.empty())
    ;
}

TEST_CASE ("save_session and load_session roundtrip") {
  const std::string path = "/tmp/llama-test-session.json";
  std::vector<Message> original = {
      {"system", "You are helpful."},
      {"user", "Hello"},
      {"assistant", "Hi there!"},
  };
  save_session(path, original);
  auto loaded = load_session(path);
  REQUIRE (loaded.size() == 3)
    ;
  CHECK (loaded[0].role == "system")
    ;
  CHECK (loaded[0].content == "You are helpful.")
    ;
  CHECK (loaded[1].role == "user")
    ;
  CHECK (loaded[1].content == "Hello")
    ;
  CHECK (loaded[2].role == "assistant")
    ;
  CHECK (loaded[2].content == "Hi there!")
    ;
  std::remove(path.c_str());
}

TEST_CASE ("session roundtrip preserves special characters") {
  const std::string path = "/tmp/llama-test-session-special.json";
  std::vector<Message> original = {
      {"user", "line1\nline2\ttab \"quoted\""},
  };
  save_session(path, original);
  auto loaded = load_session(path);
  REQUIRE (loaded.size() == 1)
    ;
  CHECK (loaded[0].content == "line1\nline2\ttab \"quoted\"")
    ;
  std::remove(path.c_str());
}

TEST_CASE ("save_session writes valid JSON") {
  const std::string path = "/tmp/llama-test-session-json.json";
  save_session(path, {{"user", "hi"}});
  std::ifstream f(path);
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  CHECK (content.find("[") == 0)
    ;
  CHECK (content.find("\"role\":\"user\"") != std::string::npos)
    ;
  CHECK (content.find("\"content\":\"hi\"") != std::string::npos)
    ;
  std::remove(path.c_str());
}
