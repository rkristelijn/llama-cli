// test_command.cpp — Unit tests for REPL command parsing and execution

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "command/command.h"

#include <doctest/doctest.h>

#include <fstream>
#include <sstream>

SCENARIO("parsing user input")
{
  GIVEN("a regular prompt")
  {
    auto result = parse_input("explain this code");
    THEN("it is parsed as a prompt")
    {
      CHECK(result.type == InputType::Prompt);
    }
  }

  GIVEN("a /read command")
  {
    auto result = parse_input("/read main.cpp");
    THEN("it is parsed as a command")
    {
      CHECK(result.type == InputType::Command);
      CHECK(result.command == "read");
      CHECK(result.arg == "main.cpp");
    }
  }

  GIVEN("a /help command")
  {
    auto result = parse_input("/help");
    THEN("it is parsed as a command")
    {
      CHECK(result.type == InputType::Command);
      CHECK(result.command == "help");
      CHECK(result.arg.empty());
    }
  }

  GIVEN("a /clear command")
  {
    auto result = parse_input("/clear");
    THEN("it is parsed as a command")
    {
      CHECK(result.type == InputType::Command);
      CHECK(result.command == "clear");
    }
  }

  GIVEN("exit keyword")
  {
    CHECK(parse_input("exit").type == InputType::Exit);
    CHECK(parse_input("quit").type == InputType::Exit);
    CHECK(parse_input("/exit").type == InputType::Exit);
    CHECK(parse_input("/quit").type == InputType::Exit);
  }

  GIVEN("empty input")
  {
    auto result = parse_input("");
    THEN("it is parsed as a prompt")
    {
      CHECK(result.type == InputType::Prompt);
    }
  }

  GIVEN("a !command")
  {
    auto result = parse_input("!ls -la");
    THEN("it is parsed as Exec")
    {
      CHECK(result.type == InputType::Exec);
      CHECK(result.arg == "ls -la");
    }
  }

  GIVEN("a !!command")
  {
    auto result = parse_input("!!make test");
    THEN("it is parsed as ExecContext")
    {
      CHECK(result.type == InputType::ExecContext);
      CHECK(result.arg == "make test");
    }
  }

  GIVEN("a /read command with path containing spaces")
  {
    auto result = parse_input("/read my file.cpp");
    THEN("the full path is captured")
    {
      CHECK(result.arg == "my file.cpp");
    }
  }
}

SCENARIO("/read command execution")
{
  GIVEN("a file that exists")
  {
    // Create a temp file
    std::ofstream tmp("/tmp/llama-test-read.txt");
    tmp << "line 1\nline 2\nline 3\n";
    tmp.close();

    std::vector<Message> history;
    std::ostringstream out;

    WHEN("/read is executed")
    {
      bool ok = cmd_read("/tmp/llama-test-read.txt", history, out);
      THEN("it succeeds")
      {
        CHECK(ok);
      }
      THEN("file is added to history as user message")
      {
        CHECK(history.size() == 1);
        CHECK(history[0].role == "user");
        CHECK(history[0].content.find("line 1") != std::string::npos);
        CHECK(history[0].content.find("llama-test-read.txt") != std::string::npos);
      }
      THEN("confirmation is printed")
      {
        CHECK(out.str().find("loaded") != std::string::npos);
      }
    }
  }

  GIVEN("a file that does not exist")
  {
    std::vector<Message> history;
    std::ostringstream out;

    WHEN("/read is executed")
    {
      bool ok = cmd_read("/tmp/nonexistent-file.xyz", history, out);
      THEN("it fails")
      {
        CHECK_FALSE(ok);
      }
      THEN("history is unchanged")
      {
        CHECK(history.empty());
      }
      THEN("error is printed")
      {
        CHECK(out.str().find("Error") != std::string::npos);
      }
    }
  }
}
