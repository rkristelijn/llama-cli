// test_repl.cpp — Unit tests for REPL loop
// Uses injectable chat function and string streams for I/O

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "repl.h"

// Mock chat: returns last user message prefixed with "echo: "
static std::string echo_chat(const std::vector<Message>& messages) { return "echo: " + messages.back().content; }

SCENARIO("REPL loop") {
  GIVEN("user types a prompt and then exits") {
    std::istringstream in("hello\nexit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("one prompt is processed") { CHECK(count == 1); }
      THEN("the response is printed") { CHECK(out.str().find("echo: hello") != std::string::npos); }
    }
  }

  GIVEN("user types exit immediately") {
    std::istringstream in("exit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("no prompts are processed") { CHECK(count == 0); }
    }
  }

  GIVEN("user types quit") {
    std::istringstream in("quit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("loop exits") { CHECK(count == 0); }
    }
  }

  GIVEN("user types empty lines") {
    std::istringstream in("\n\nhello\nexit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("empty lines are skipped") { CHECK(count == 1); }
    }
  }

  GIVEN("user types multiple prompts") {
    std::istringstream in("first\nsecond\nthird\nexit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("all prompts are processed") { CHECK(count == 3); }
      THEN("all responses are printed") {
        CHECK(out.str().find("echo: first") != std::string::npos);
        CHECK(out.str().find("echo: second") != std::string::npos);
        CHECK(out.str().find("echo: third") != std::string::npos);
      }
    }
  }

  GIVEN("conversation history is maintained") {
    // Mock that checks history length grows
    int call_count = 0;
    auto history_chat = [&](const std::vector<Message>& msgs) {
      call_count++;
      // First call: 1 message, second: 3 (user+assistant+user)
      if (call_count == 1) CHECK(msgs.size() == 1);
      if (call_count == 2) CHECK(msgs.size() == 3);
      return "ok";
    };

    std::istringstream in("first\nsecond\nexit\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      run_repl(history_chat, "", in, out);
      THEN("history grows with each exchange") { CHECK(call_count == 2); }
    }
  }

  GIVEN("stdin reaches EOF") {
    std::istringstream in("hello\n");
    std::ostringstream out;

    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("it exits cleanly") { CHECK(count == 1); }
    }
  }
}
