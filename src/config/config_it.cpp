/**
 * @file config_it.cpp
 * @brief Integration tests for configuration loading (--files flag, ADR-030)
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "config/config.h"

TEST_SUITE("config --files flag (ADR-030)")
{
  TEST_CASE("single file with --files=path")
  {
    const char* argv[] = {"prog", "--files=/tmp/test.txt", "prompt"};
    Config c = load_cli(3, argv);
    CHECK(c.files.size() == 1);
    CHECK(c.files[0] == "/tmp/test.txt");
    CHECK(c.prompt == "prompt");
    CHECK(c.mode == Mode::Sync);
  }

  TEST_CASE("multiple files space-separated")
  {
    const char* argv[] = {"prog", "--files=/tmp/a.txt /tmp/b.txt", "prompt"};
    Config c = load_cli(3, argv);
    CHECK(c.files.size() == 2);
    CHECK(c.files[0] == "/tmp/a.txt");
    CHECK(c.files[1] == "/tmp/b.txt");
    CHECK(c.prompt == "prompt");
    CHECK(c.mode == Mode::Sync);
  }

  TEST_CASE("--files with space syntax")
  {
    const char* argv[] = {"prog", "--files", "/tmp/test.txt", "prompt"};
    Config c = load_cli(4, argv);
    CHECK(c.files.size() == 1);
    CHECK(c.files[0] == "/tmp/test.txt");
    CHECK(c.prompt == "prompt");
    CHECK(c.mode == Mode::Sync);
  }

  TEST_CASE("--files consumes multiple argv paths")
  {
    const char* argv[] = {"prog", "--files", "/tmp/a.txt", "/tmp/b.txt", "prompt"};
    Config c = load_cli(5, argv);
    CHECK(c.files.size() == 2);
    CHECK(c.files[0] == "/tmp/a.txt");
    CHECK(c.files[1] == "/tmp/b.txt");
    CHECK(c.prompt == "prompt");
    CHECK(c.mode == Mode::Sync);
  }

  TEST_CASE("no prompt after files")
  {
    const char* argv[] = {"prog", "--files=/tmp/test.txt"};
    Config c = load_cli(2, argv);
    CHECK(c.files.size() == 1);
    CHECK(c.prompt.empty());
    CHECK(c.mode == Mode::Sync);
  }

  TEST_CASE("files with other options")
  {
    const char* argv[] = {"prog", "--model=gemma4:26b", "--files=/tmp/test.txt", "prompt"};
    Config c = load_cli(4, argv);
    CHECK(c.model == "gemma4:26b");
    CHECK(c.files.size() == 1);
    CHECK(c.files[0] == "/tmp/test.txt");
    CHECK(c.prompt == "prompt");
  }
}
