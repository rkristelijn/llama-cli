/**
 * @file test_integration.cpp
 * @brief Integration tests — full REPL session flows.
 * @see test/features/ for Gherkin specs
 * @see docs/adr/adr-017-integration-tests.md
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <fstream>
#include <sstream>
#include <vector>

#include "config.h"
#include "repl.h"

// Smart mock: echoes last user message, tracks call count and history
struct MockLLM {
  int calls = 0;
  std::vector<Message> last_history;
  std::string fixed_response;

  std::string operator()(const std::vector<Message>& msgs) {
    calls++;
    last_history = msgs;
    if (!fixed_response.empty()) {
      return fixed_response;
    }
    return "echo: " + msgs.back().content;
  }
};

// Config with empty system prompt for cleaner assertions
static Config test_cfg() {
  Config c;
  c.system_prompt = "";
  return c;
}

// --- Feature: Full conversation with history ---

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
      THEN("the LLM sees full history on second call") {
        // History: user "hello", assistant "echo: hello", user "what did I say?"
        CHECK(llm.last_history.size() == 3);
      }
    }
  }
}

// --- Feature: Command chaining ---

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

// --- Feature: Runtime options persist ---

SCENARIO("Runtime options persist across prompts") {
  GIVEN("the LLM echoes user messages") {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    // Turn markdown off, then send bold text
    std::istringstream in("/set markdown\n**bold text**\nexit\n");
    std::ostringstream out;

    WHEN("the user disables markdown then sends bold text") {
      run_repl(chat, test_cfg(), in, out);

      THEN("markdown toggle is confirmed") { CHECK(out.str().find("[markdown off]") != std::string::npos); }
      THEN("output contains raw asterisks (not ANSI bold)") {
        // With markdown off, **bold** should appear as-is in the echo
        CHECK(out.str().find("**bold text**") != std::string::npos);
      }
    }
  }
}

// --- Feature: Version and help ---

SCENARIO("Set shows all options") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set\nexit\n");
  std::ostringstream out;

  WHEN("the user runs /set") {
    run_repl(chat, test_cfg(), in, out);
    THEN("all options are listed") {
      CHECK(out.str().find("markdown") != std::string::npos);
      CHECK(out.str().find("color") != std::string::npos);
      CHECK(out.str().find("bofh") != std::string::npos);
    }
  }
}

SCENARIO("Set toggles color and bofh") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set color\n/set bofh\n/set\nexit\n");
  std::ostringstream out;

  WHEN("the user toggles color and bofh then checks") {
    run_repl(chat, test_cfg(), in, out);
    THEN("toggles are confirmed") {
      CHECK(out.str().find("[color on]") != std::string::npos);
      CHECK(out.str().find("[bofh on]") != std::string::npos);
    }
  }
}

SCENARIO("Set unknown option shows error") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/set foobar\nexit\n");
  std::ostringstream out;

  WHEN("the user runs /set foobar") {
    run_repl(chat, test_cfg(), in, out);
    THEN("error is shown") { CHECK(out.str().find("Unknown option") != std::string::npos); }
  }
}

// --- Feature: Version and help ---

SCENARIO("Version and help commands") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
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

// --- Feature: Clear history ---

SCENARIO("Clear history resets conversation") {
  GIVEN("the LLM echoes user messages") {
    MockLLM llm;
    auto chat = [&](const std::vector<Message>& m) { return llm(m); };
    std::istringstream in("remember this\n/clear\nwhat did I say?\nexit\n");
    std::ostringstream out;

    WHEN("the user sends a message, clears, then asks") {
      run_repl(chat, test_cfg(), in, out);

      THEN("the LLM does not see the first message after clear") {
        // After /clear, history is empty, so last call only has "what did I say?"
        CHECK(llm.last_history.size() == 1);
        CHECK(llm.last_history[0].content == "what did I say?");
      }
    }
  }
}

// --- Feature: Write annotation ---

SCENARIO("Write annotation with confirm and skip") {
  const std::string test_path = "/tmp/llama-integration-test.txt";
  std::remove(test_path.c_str());

  GIVEN("the LLM responds with a write annotation") {
    auto write_chat = [](const std::vector<Message>&) {
      return "Here: <write file=\"/tmp/llama-integration-test.txt\">integration test</write>";
    };

    WHEN("the user confirms with y") {
      std::istringstream in("write it\ny\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);

      THEN("the file is written") {
        std::ifstream f(test_path);
        CHECK(f.is_open());
        std::string content;
        std::getline(f, content);
        CHECK(content == "integration test");
      }
    }

    WHEN("the user declines with n") {
      std::remove(test_path.c_str());
      std::istringstream in("write it\nn\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);

      THEN("the file is not written") {
        std::ifstream f(test_path);
        CHECK_FALSE(f.is_open());
      }
      THEN("[skipped] is shown") { CHECK(out.str().find("[skipped]") != std::string::npos); }
    }
  }
  std::remove(test_path.c_str());
}

// --- Feature: Exec annotation ---

SCENARIO("Exec annotation with confirm and skip") {
  GIVEN("the LLM responds with an exec annotation") {
    int call_count = 0;
    auto exec_chat = [&](const std::vector<Message>& /*msgs*/) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "Let me check: <exec>echo integration123</exec>";
      }
      return "I see the output";
    };

    WHEN("the user confirms with y") {
      std::istringstream in("do it\ny\nexit\n");
      std::ostringstream out;
      run_repl(exec_chat, test_cfg(), in, out);

      THEN("command output is shown") { CHECK(out.str().find("integration123") != std::string::npos); }
      THEN("LLM gets a follow-up") { CHECK(call_count == 2); }
    }

    WHEN("the user declines with n") {
      call_count = 0;
      std::istringstream in("do it\nn\nexit\n");
      std::ostringstream out;
      run_repl(exec_chat, test_cfg(), in, out);

      THEN("[skipped] is shown") { CHECK(out.str().find("[skipped]") != std::string::npos); }
      THEN("no follow-up call") { CHECK(call_count == 1); }
    }
  }
}

// --- Feature: Unknown command ---

SCENARIO("Unknown command shows error") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("/foobar\nexit\n");
  std::ostringstream out;

  WHEN("the user runs /foobar") {
    run_repl(chat, test_cfg(), in, out);
    THEN("error is shown") { CHECK(out.str().find("Unknown command") != std::string::npos); }
    THEN("no LLM call") { CHECK(llm.calls == 0); }
  }
}

// --- Feature: Shell commands ! and !! ---

SCENARIO("Shell command with ! and !!") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };

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

// --- Feature: Write with show ---

SCENARIO("Write annotation with show then confirm") {
  const std::string test_path = "/tmp/llama-integration-show.txt";
  std::remove(test_path.c_str());

  GIVEN("the LLM responds with a write annotation") {
    auto write_chat = [](const std::vector<Message>&) {
      return "Here: <write file=\"/tmp/llama-integration-show.txt\">show content</write>";
    };

    WHEN("the user types s then y") {
      std::istringstream in("write it\ns\ny\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);

      THEN("content is previewed") { CHECK(out.str().find("show content") != std::string::npos); }
      THEN("file is written") {
        std::ifstream f(test_path);
        CHECK(f.is_open());
      }
    }
  }
  std::remove(test_path.c_str());
}

// --- Feature: Empty lines ---

SCENARIO("Empty lines are skipped") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("\n\n\nhello\n\nexit\n");
  std::ostringstream out;

  WHEN("the user sends empty lines between prompts") {
    run_repl(chat, test_cfg(), in, out);
    THEN("only one LLM call is made") { CHECK(llm.calls == 1); }
  }
}

// --- Feature: Quit ---

SCENARIO("Quit exits the REPL") {
  MockLLM llm;
  auto chat = [&](const std::vector<Message>& m) { return llm(m); };
  std::istringstream in("quit\n");
  std::ostringstream out;

  WHEN("the user types quit") {
    int count = run_repl(chat, test_cfg(), in, out);
    THEN("REPL exits with 0 prompts") { CHECK(count == 0); }
    THEN("no LLM calls") { CHECK(llm.calls == 0); }
  }
}

// --- Feature: Markdown rendering ---

SCENARIO("Markdown rendering on vs off") {
  GIVEN("the LLM responds with markdown") {
    auto md_chat = [](const std::vector<Message>&) {
      return "# Title\n**bold** and *italic*\n- item one\n- item two\n```\ncode block\n```";
    };

    WHEN("markdown is on (default)") {
      std::istringstream in("render\nexit\n");
      std::ostringstream out;
      Config cfg = test_cfg();
      cfg.no_color = true;  // no ANSI but markdown still processes
      run_repl(md_chat, cfg, in, out);

      THEN("output contains the response") {
        CHECK(out.str().find("Title") != std::string::npos);
        CHECK(out.str().find("bold") != std::string::npos);
        CHECK(out.str().find("item") != std::string::npos);
      }
    }

    WHEN("markdown is off via /set") {
      std::istringstream in("/set markdown\nrender\nexit\n");
      std::ostringstream out;
      run_repl(md_chat, test_cfg(), in, out);

      THEN("raw markdown is preserved") {
        CHECK(out.str().find("# Title") != std::string::npos);
        CHECK(out.str().find("**bold**") != std::string::npos);
      }
    }
  }
}

// --- Feature: BOFH mode config ---

SCENARIO("BOFH mode via --why-so-serious") {
  GIVEN("config with bofh enabled") {
    Config cfg = test_cfg();
    cfg.bofh = true;

    WHEN("the REPL starts and user checks /set") {
      MockLLM llm;
      auto chat = [&](const std::vector<Message>& m) { return llm(m); };
      std::istringstream in("/set\nexit\n");
      std::ostringstream out;
      run_repl(chat, cfg, in, out);

      THEN("bofh shows as on") { CHECK(out.str().find("bofh      on") != std::string::npos); }
    }
  }
}

// --- Feature: --no-color config ---

SCENARIO("No-color config flag") {
  GIVEN("config with no_color enabled") {
    Config cfg = test_cfg();
    cfg.no_color = true;

    WHEN("the REPL starts and user checks /set") {
      MockLLM llm;
      auto chat = [&](const std::vector<Message>& m) { return llm(m); };
      std::istringstream in("/set\nexit\n");
      std::ostringstream out;
      run_repl(chat, cfg, in, out);

      THEN("color shows as off") { CHECK(out.str().find("color     off") != std::string::npos); }
    }
  }
}

// --- Unit: render_markdown ---

#include "tui.h"

SCENARIO("Markdown renderer") {
  GIVEN("color is enabled") {
    WHEN("rendering a heading") {
      auto result = tui::render_markdown("# Hello", true);
      THEN("heading text is present without #") {
        CHECK(result.find("Hello") != std::string::npos);
        CHECK(result.find("# ") == std::string::npos);
      }
    }

    WHEN("rendering bold") {
      auto result = tui::render_markdown("**bold**", true);
      THEN("bold text is present without **") {
        CHECK(result.find("bold") != std::string::npos);
        CHECK(result.find("**") == std::string::npos);
      }
    }

    WHEN("rendering italic") {
      auto result = tui::render_markdown("*italic*", true);
      THEN("italic text is present without *") {
        CHECK(result.find("italic") != std::string::npos);
        CHECK(result.find("*italic*") == std::string::npos);
      }
    }

    WHEN("rendering inline code") {
      auto result = tui::render_markdown("`code`", true);
      THEN("code text is present without backticks") {
        CHECK(result.find("code") != std::string::npos);
        CHECK(result.find("`code`") == std::string::npos);
      }
    }

    WHEN("rendering a bullet list") {
      auto result = tui::render_markdown("- item one\n- item two", true);
      THEN("bullets are converted") {
        CHECK(result.find("•") != std::string::npos);
        CHECK(result.find("item one") != std::string::npos);
        CHECK(result.find("item two") != std::string::npos);
      }
    }

    WHEN("rendering a numbered list") {
      auto result = tui::render_markdown("1. first\n2. second", true);
      THEN("numbers are preserved") {
        CHECK(result.find("1.") != std::string::npos);
        CHECK(result.find("first") != std::string::npos);
      }
    }

    WHEN("rendering a code block") {
      auto result = tui::render_markdown("```\nint x = 1;\n```", true);
      THEN("code content is present") { CHECK(result.find("int x = 1;") != std::string::npos); }
    }
  }

  GIVEN("color is disabled") {
    WHEN("rendering markdown") {
      auto result = tui::render_markdown("# Hello\n**bold**", false);
      THEN("text is returned unchanged") { CHECK(result == "# Hello\n**bold**"); }
    }
  }
}
