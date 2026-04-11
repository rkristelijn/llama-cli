// test_repl.cpp — Unit tests for REPL loop
// Uses injectable chat function and string streams for I/O

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <fstream>
#include <sstream>

#include "repl.h"

// Mock chat: returns last user message prefixed with "echo: "
static std::string echo_chat(const std::vector<Message>& messages) { return "echo: " + messages.back().content; }

SCENARIO("REPL basic flow") {
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
    WHEN("the REPL runs") { CHECK(run_repl(echo_chat, "", in, out) == 0); }
  }

  GIVEN("user types empty lines") {
    std::istringstream in("\n\nhello\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") { CHECK(run_repl(echo_chat, "", in, out) == 1); }
  }

  GIVEN("multiple prompts") {
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

  GIVEN("stdin reaches EOF") {
    std::istringstream in("hello\n");
    std::ostringstream out;
    WHEN("the REPL runs") { CHECK(run_repl(echo_chat, "", in, out) == 1); }
  }
}

SCENARIO("REPL conversation history") {
  GIVEN("two prompts are sent") {
    int call_count = 0;
    auto history_chat = [&](const std::vector<Message>& msgs) {
      call_count++;
      if (call_count == 1) {
        CHECK(msgs.size() == 1);
      }
      if (call_count == 2) {
        CHECK(msgs.size() == 3);
      }
      return "ok";
    };
    std::istringstream in("first\nsecond\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(history_chat, "", in, out);
      THEN("history grows") { CHECK(call_count == 2); }
    }
  }

  GIVEN("a system prompt is provided") {
    auto sys_chat = [](const std::vector<Message>& msgs) {
      CHECK(msgs[0].role == "system");
      CHECK(msgs[0].content == "be helpful");
      return "ok";
    };
    std::istringstream in("hi\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") { run_repl(sys_chat, "be helpful", in, out); }
  }
}

SCENARIO("REPL slash commands") {
  GIVEN("user types /help") {
    std::istringstream in("/help\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      int count = run_repl(echo_chat, "", in, out);
      THEN("no prompt is sent") { CHECK(count == 0); }
      THEN("help text is shown") {
        CHECK(out.str().find("/read") != std::string::npos);
        CHECK(out.str().find("/clear") != std::string::npos);
      }
    }
  }

  GIVEN("user types /clear") {
    auto clear_chat = [](const std::vector<Message>& msgs) {
      CHECK(msgs.size() == 1);  // only the new message, history was cleared
      return "ok";
    };
    std::istringstream in("first\n/clear\nsecond\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(clear_chat, "", in, out);
      THEN("cleared message is shown") { CHECK(out.str().find("[history cleared]") != std::string::npos); }
    }
  }

  GIVEN("user types an unknown command") {
    std::istringstream in("/foobar\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(echo_chat, "", in, out);
      THEN("error is shown") { CHECK(out.str().find("Unknown command: /foobar") != std::string::npos); }
    }
  }
}

SCENARIO("REPL write annotations") {
  // Mock that returns a <write> annotation
  auto write_chat = [](const std::vector<Message>&) {
    return "Here:\n<write file=\"/tmp/llama-repl-test.txt\">test content</write>\nDone.";
  };

  GIVEN("LLM responds with a write annotation and user confirms") {
    std::istringstream in("create file\ny\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(write_chat, "", in, out);
      THEN("annotation is stripped from output") { CHECK(out.str().find("<write") == std::string::npos); }
      THEN("proposed summary is shown") { CHECK(out.str().find("[proposed: write") != std::string::npos); }
      THEN("file is written") {
        std::ifstream f("/tmp/llama-repl-test.txt");
        CHECK(f.is_open());
        std::string content;
        std::getline(f, content);
        CHECK(content == "test content");
      }
      THEN("confirmation is shown") { CHECK(out.str().find("[wrote") != std::string::npos); }
    }
    std::remove("/tmp/llama-repl-test.txt");
  }

  GIVEN("LLM responds with a write annotation and user declines") {
    std::istringstream in("create file\nn\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(write_chat, "", in, out);
      THEN("file is not written") {
        std::ifstream f("/tmp/llama-repl-test.txt");
        CHECK_FALSE(f.is_open());
      }
      THEN("skipped message is shown") { CHECK(out.str().find("[skipped]") != std::string::npos); }
    }
  }

  GIVEN("LLM responds with a write annotation and user shows first") {
    std::istringstream in("create file\ns\ny\nexit\n");
    std::ostringstream out;
    WHEN("the REPL runs") {
      run_repl(write_chat, "", in, out);
      THEN("content is previewed") { CHECK(out.str().find("test content") != std::string::npos); }
      THEN("file is written after show+confirm") { CHECK(out.str().find("[wrote") != std::string::npos); }
    }
    std::remove("/tmp/llama-repl-test.txt");
  }
}
