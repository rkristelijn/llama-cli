/**
 * @file options_it.cpp
 * @brief Integration tests: /set, toggles, --no-color, --why-so-serious
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "test_helpers.h"

SCENARIO("Set shows all options")
{
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set\nexit\n");
  std::ostringstream out;
  WHEN("the user runs /set")
  {
    run_repl(chat, test_cfg(), in, out);
    THEN("all options are listed")
    {
      CHECK(out.str().find("markdown") != std::string::npos);
      CHECK(out.str().find("color") != std::string::npos);
      CHECK(out.str().find("bofh") != std::string::npos);
    }
  }
}

SCENARIO("Set toggles color and bofh")
{
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set color\n/set bofh\n/set\nexit\n");
  std::ostringstream out;
  Config cfg = test_cfg();
  cfg.no_color = true;  // Force initial color off
  WHEN("the user toggles color and bofh then checks")
  {
    run_repl(chat, cfg, in, out);
    THEN("toggles are confirmed")
    {
      CHECK(out.str().find("[color on]") != std::string::npos);
      CHECK(out.str().find("[bofh on]") != std::string::npos);
    }
  }
}

SCENARIO("Set unknown option shows error")
{
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set foobar\nexit\n");
  std::ostringstream out;
  WHEN("the user runs /set foobar")
  {
    run_repl(chat, test_cfg(), in, out);
    THEN("error is shown")
    {
      CHECK(out.str().find("Unknown option") != std::string::npos);
    }
  }
}

SCENARIO("Runtime options persist across prompts")
{
  GIVEN("the LLM echoes user messages")
  {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    std::istringstream in("/set markdown\n**bold text**\nexit\n");
    std::ostringstream out;
    WHEN("the user disables markdown then sends bold text")
    {
      run_repl(chat, test_cfg(), in, out);
      THEN("markdown toggle is confirmed")
      {
        CHECK(out.str().find("[markdown off]") != std::string::npos);
      }
      THEN("output contains raw asterisks")
      {
        CHECK(out.str().find("**bold text**") != std::string::npos);
      }
    }
  }
}

SCENARIO("BOFH mode via --why-so-serious")
{
  Config cfg = test_cfg();
  cfg.bofh = true;
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set\nexit\n");
  std::ostringstream out;
  WHEN("the REPL starts with bofh config")
  {
    run_repl(chat, cfg, in, out);
    THEN("bofh shows as on")
    {
      CHECK(out.str().find("bofh      on") != std::string::npos);
    }
  }
}

SCENARIO("No-color config flag")
{
  Config cfg = test_cfg();
  cfg.no_color = true;
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set\nexit\n");
  std::ostringstream out;
  WHEN("the REPL starts with no_color config")
  {
    run_repl(chat, cfg, in, out);
    THEN("color shows as off")
    {
      CHECK(out.str().find("color     off") != std::string::npos);
    }
  }
}
