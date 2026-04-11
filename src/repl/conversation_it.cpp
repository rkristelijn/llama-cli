/**
 * @file test_conversation.cpp
 * @brief Integration tests: chat, history, clear, command chaining
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "test_helpers.h"

SCENARIO("Full conversation with history") {
  GIVEN("the LLM echoes user messages") {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    std::istringstream in("hello\nwhat did I say?\nexit\n");
    std::ostringstream out;
    WHEN("the user sends two messages") {
      run_repl(chat, test_cfg(), in, out);
      THEN("the LLM is called twice") { CHECK(llm.calls == 2); }
      THEN("both responses are printed") {
        CHECK(out.str().find("echo: hello") != std::string::npos);
        CHECK(out.str().find("echo: what did I say?") != std::string::npos);
      }
      THEN("the LLM sees full history on second call") { CHECK(llm.last_history.size() == 3); }
    }
  }
}

SCENARIO("Command chaining — read file then ask about it") {
  GIVEN("the LLM echoes user messages") {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    std::istringstream in("!!echo file content here\nwhat was in the file?\nexit\n");
    std::ostringstream out;
    WHEN("the user runs !! then asks about it") {
      run_repl(chat, test_cfg(), in, out);
      THEN("the LLM sees command output in history") {
        bool found = false;
        for (const auto& m : llm.last_history) {
          if (m.content.find("file content here") != std::string::npos) {
            found = true;
          }
        }
        CHECK(found);
      }
    }
  }
}

SCENARIO("Clear history resets conversation") {
  GIVEN("the LLM echoes user messages") {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    std::istringstream in("remember this\n/clear\nwhat did I say?\nexit\n");
    std::ostringstream out;
    WHEN("the user sends a message, clears, then asks") {
      run_repl(chat, test_cfg(), in, out);
      THEN("the LLM does not see the first message after clear") {
        CHECK(llm.last_history.size() == 1);
        CHECK(llm.last_history[0].content == "what did I say?");
      }
    }
  }
}
