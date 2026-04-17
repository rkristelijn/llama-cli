/**
 * @file commands_it.cpp
 * @brief Integration tests: !, !!, /help, /version, unknown, empty, quit
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "test_helpers.h"

SCENARIO("Version and help commands") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m, Trace*) { return llm(m); };
  std::istringstream in("/version\n/help\nexit\n");
  std::ostringstream out;
  WHEN("the user runs /version and /help") {
    run_repl(chat, test_cfg(), in, out);
    THEN("/version shows llama-cli") { CHECK(out.str().find("llama-cli") != std::string::npos); }
    THEN("/help shows /set and !command") {
      CHECK(out.str().find("/set") != std::string::npos);
      CHECK(out.str().find("!command") != std::string::npos);
    }
    THEN("no LLM calls were made") { CHECK(llm.calls == 0); }
  }
}

SCENARIO("Unknown command shows error") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m, Trace*) { return llm(m); };
  std::istringstream in("/foobar\nexit\n");
  std::ostringstream out;
  WHEN("the user runs /foobar") {
    run_repl(chat, test_cfg(), in, out);
    THEN("error is shown") { CHECK(out.str().find("Unknown command") != std::string::npos); }
    THEN("no LLM call") { CHECK(llm.calls == 0); }
  }
}

SCENARIO("Shell command with ! and !!") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m, Trace*) { return llm(m); };

  GIVEN("user runs ! command") {
    std::istringstream in("!echo direct\nexit\n");
    std::ostringstream out;
    run_repl(chat, test_cfg(), in, out);
    THEN("output is shown") { CHECK(out.str().find("direct") != std::string::npos); }
    THEN("not sent to LLM") { CHECK(llm.calls == 0); }
  }

  GIVEN("user runs !! command then asks about it") {
    std::istringstream in("!!echo context\nwhat was that?\nexit\n");
    std::ostringstream out;
    run_repl(chat, test_cfg(), in, out);
    THEN("output is shown") { CHECK(out.str().find("context") != std::string::npos); }
    THEN("LLM sees it in history") {
      bool found = false;
      for (const auto& m : llm.last_history) {
        if (m.content.find("context") != std::string::npos) {
          found = true;
        }
      }
      CHECK(found);
    }
  }
}

SCENARIO("Empty lines are skipped") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m, Trace*) { return llm(m); };
  std::istringstream in("\n\n\nhello\n\nexit\n");
  std::ostringstream out;
  WHEN("the user sends empty lines between prompts") {
    run_repl(chat, test_cfg(), in, out);
    THEN("only one LLM call is made") { CHECK(llm.calls == 1); }
  }
}

SCENARIO("Quit exits the REPL") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m, Trace*) { return llm(m); };
  std::istringstream in("quit\n");
  std::ostringstream out;
  WHEN("the user types quit") {
    int count = run_repl(chat, test_cfg(), in, out);
    THEN("REPL exits with 0 prompts") { CHECK(count == 0); }
    THEN("no LLM calls") { CHECK(llm.calls == 0); }
  }
}
