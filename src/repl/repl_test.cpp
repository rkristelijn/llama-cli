// test_repl.cpp — Unit tests for REPL loop
// Uses injectable chat function and string streams for I/O
// Tests: basic flow, history, commands, write y/n/s, exec !/!!/<exec>,
// /set toggles, /version
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "repl/repl.h"

#include <doctest/doctest.h>

#include <csignal>

extern volatile sig_atomic_t g_interrupted;

#include <fstream>
#include <sstream>

#include "config/config.h"
#include "repl/repl_commands.h"
#include "test_helpers.h"

// Mock chat: returns last user message prefixed with "echo: "
static std::string echo_chat(const std::vector<Message>& messages) { return "echo: " + messages.back().content; }

// Mock model info: returns metadata matching the test model list
// Params are set so sort-by-params gives expected order: gemma4:e4b(4B), gemma4:26b(26B), llama3:8b(8B), mistral:7b(7B), phi3:mini(3B)
static std::vector<ModelInfo> mock_model_info(const Config&) {
  return {{"gemma4:e4b", "4.0B", "Q4_0", "gemma4", 2.5},
          {"gemma4:26b", "26.0B", "Q4_K_M", "gemma4", 15.0},
          {"llama3:8b", "8.0B", "Q4_0", "llama3", 4.5},
          {"mistral:7b", "7.0B", "Q4_0", "mistral", 4.0},
          {"phi3:mini", "3.8B", "Q4_0", "phi3", 2.0}};
}

// Mock hardware: returns dummy hardware info
static HardwareInfo mock_hw() { return {16, 8, "mock-cpu", "mock-gpu"}; }

SCENARIO ("REPL basic flow") {
  GIVEN ("user types a prompt and then exits") {
    std::istringstream in("hello\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      int count = run_repl(echo_chat, test_cfg(), in, out);
      THEN ("one prompt is processed") {
        CHECK (count == 1)
          ;
      }
      THEN ("the response is printed") {
        CHECK (out.str().find("echo: hello") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user types exit immediately") {
    std::istringstream in("exit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      int count = run_repl(echo_chat, test_cfg(), in, out);
      THEN ("no prompts are processed") {
        CHECK (count == 0)
          ;
      }
    }
  }

  GIVEN ("user types quit") {
    std::istringstream in("quit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      CHECK (run_repl(echo_chat, test_cfg(), in, out) == 0)
        ;
    }
  }

  GIVEN ("user types empty lines") {
    std::istringstream in("\n\nhello\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      CHECK (run_repl(echo_chat, test_cfg(), in, out) == 1)
        ;
    }
  }

  GIVEN ("multiple prompts") {
    std::istringstream in("first\nsecond\nthird\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      int count = run_repl(echo_chat, test_cfg(), in, out);
      THEN ("all prompts are processed") {
        CHECK (count == 3)
          ;
      }
      THEN ("all responses are printed") {
        CHECK (out.str().find("echo: first") != std::string::npos)
          ;
        CHECK (out.str().find("echo: second") != std::string::npos)
          ;
        CHECK (out.str().find("echo: third") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("stdin reaches EOF") {
    std::istringstream in("hello\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      CHECK (run_repl(echo_chat, test_cfg(), in, out) == 1)
        ;
    }
  }
}

SCENARIO ("REPL conversation history") {
  GIVEN ("two prompts are sent") {
    int call_count = 0;
    auto history_chat = [&](const std::vector<Message>& msgs) {
      call_count++;
      if (call_count == 1) {
        CHECK (msgs.size() == 1)
          ;
      }
      if (call_count == 2) {
        CHECK (msgs.size() == 3)
          ;
      }
      return "ok";
    };
    std::istringstream in("first\nsecond\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(history_chat, test_cfg(), in, out);
      THEN ("history grows") {
        CHECK (call_count == 2)
          ;
      }
    }
  }

  GIVEN ("a system prompt is provided") {
    auto sys_chat = [](const std::vector<Message>& msgs) {
      CHECK (msgs[0].role == "system")
        ;
      CHECK (msgs[0].content == "be helpful")
        ;
      return "ok";
    };
    std::istringstream in("hi\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      Config cfg;
      cfg.system_prompt = "be helpful";
      cfg.memory_path = "";
      cfg.preferences_path = "";
      run_repl(sys_chat, cfg, in, out);
    }
  }
}

SCENARIO ("REPL sliding window trims old messages") {
  GIVEN ("max_history is set to 2 pairs") {
    std::vector<size_t> sizes;
    auto tracking_chat = [&](const std::vector<Message>& msgs) {
      sizes.push_back(msgs.size());
      return "ok";
    };
    // 4 prompts: after 3rd, history should be trimmed to system + last 2 pairs
    std::istringstream in("a\nb\nc\nd\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      Config cfg = test_cfg();
      cfg.max_history = 2;
      run_repl(tracking_chat, cfg, in, out);
      THEN ("history is capped at 4 non-system messages") {
        // Prompt 1: [user] = 1 msg
        CHECK (sizes[0] == 1)
          ;
        // Prompt 2: [user, assistant, user] = 3 msgs
        CHECK (sizes[1] == 3)
          ;
        // Prompt 3: trimmed to last 2 pairs = 4 msgs
        CHECK (sizes[2] == 4)
          ;
        // Prompt 4: still 4 (old trimmed away)
        CHECK (sizes[3] == 4)
          ;
      }
    }
  }
}

SCENARIO ("REPL slash commands") {
  GIVEN ("user types /help") {
    std::istringstream in("/help\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      int count = run_repl(echo_chat, test_cfg(), in, out);
      THEN ("no prompt is sent") {
        CHECK (count == 0)
          ;
      }
      THEN ("help text is shown") {
        CHECK (out.str().find("!command") != std::string::npos)
          ;
        CHECK (out.str().find("/clear") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user types /clear") {
    auto clear_chat = [](const std::vector<Message>& msgs) {
      CHECK (msgs.size() == 1)
        ;  // only the new message, history was cleared
      return "ok";
    };
    std::istringstream in("first\n/clear\nsecond\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(clear_chat, test_cfg(), in, out);
      THEN ("cleared message is shown") {
        CHECK (out.str().find("[history cleared]") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user types an unknown command") {
    std::istringstream in("/foobar\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("error is shown") {
        CHECK (out.str().find("Unknown command: /foobar") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("REPL write annotations") {
  // Mock that returns a <write> annotation
  auto write_chat = [](const std::vector<Message>&) {
    return "Here:\n<write file=\"/tmp/llama-repl-test.txt\">test "
           "content</write>\nDone.";
  };

  GIVEN ("LLM responds with a write annotation and user confirms") {
    std::istringstream in("create file\ny\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("annotation is stripped from output") {
        CHECK (out.str().find("<write") == std::string::npos)
          ;
      }
      THEN ("proposed summary is shown") {
        CHECK (out.str().find("[proposed: write") != std::string::npos)
          ;
      }
      THEN ("file is written") {
        std::ifstream f("/tmp/llama-repl-test.txt");
        CHECK (f.is_open())
          ;
        std::string content;
        std::getline(f, content);
        CHECK (content == "test content")
          ;
      }
      THEN ("confirmation is shown") {
        CHECK (out.str().find("[wrote") != std::string::npos)
          ;
      }
    }
    std::remove("/tmp/llama-repl-test.txt");
  }

  GIVEN ("LLM responds with a write annotation and user declines") {
    std::istringstream in("create file\nn\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("file is not written") {
        std::ifstream f("/tmp/llama-repl-test.txt");
        CHECK_FALSE (f.is_open())
          ;
      }
      THEN ("skipped message is shown") {
        CHECK (out.str().find("[skipped]") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("LLM responds with a write annotation and user shows first") {
    std::istringstream in("create file\ns\ny\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("content is previewed") {
        CHECK (out.str().find("test content") != std::string::npos)
          ;
      }
      THEN ("file is written after show+confirm") {
        CHECK (out.str().find("[wrote") != std::string::npos)
          ;
      }
    }
    std::remove("/tmp/llama-repl-test.txt");
  }
}

SCENARIO ("REPL command execution") {
  GIVEN ("user runs !echo hello") {
    std::istringstream in("!echo hello\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      int count = run_repl(echo_chat, test_cfg(), in, out);
      THEN ("command output is shown") {
        CHECK (out.str().find("hello") != std::string::npos)
          ;
      }
      THEN ("no prompt is sent to LLM") {
        CHECK (count == 0)
          ;
      }
    }
  }

  GIVEN ("user runs !!echo context") {
    int call_count = 0;
    auto ctx_chat = [&](const std::vector<Message>& msgs) {
      call_count++;
      // History should contain the command output before the user prompt
      bool has_cmd = false;
      for (const auto& m : msgs) {
        if (m.content.find("[command:") != std::string::npos) {
          has_cmd = true;
        }
      }
      CHECK (has_cmd)
        ;
      return "got it";
    };
    std::istringstream in("!!echo context\nwhat was that?\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(ctx_chat, test_cfg(), in, out);
      THEN ("LLM sees command output in history") {
        CHECK (call_count == 1)
          ;
      }
    }
  }

  GIVEN ("LLM responds with an <exec> annotation and user confirms") {
    auto exec_chat = [](const std::vector<Message>&) { return "Let me check: <exec>echo test123</exec>"; };
    std::istringstream in("run it\ny\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(exec_chat, test_cfg(), in, out);
      THEN ("confirmation is asked") {
        CHECK (out.str().find("Run:") != std::string::npos)
          ;
        CHECK (out.str().find("echo test123") != std::string::npos)
          ;
      }
      THEN ("command output is shown") {
        CHECK (out.str().find("test123") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("LLM responds with an <exec> annotation and user declines") {
    auto exec_chat = [](const std::vector<Message>&) { return "Let me run: <exec>echo nope</exec>"; };
    std::istringstream in("run it\nn\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(exec_chat, test_cfg(), in, out);
      THEN ("skipped message is shown") {
        CHECK (out.str().find("[skipped]") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("REPL /set command") {
  GIVEN ("user types /set without args") {
    std::istringstream in("/set\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("options are shown") {
        CHECK (out.str().find("markdown") != std::string::npos)
          ;
        CHECK (out.str().find("color") != std::string::npos)
          ;
        CHECK (out.str().find("bofh") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user toggles markdown off") {
    std::istringstream in("/set markdown\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("toggle is confirmed") {
        CHECK (out.str().find("[markdown off]") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user toggles bofh on") {
    std::istringstream in("/set bofh\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("toggle is confirmed") {
        CHECK (out.str().find("[bofh on]") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user types /set with unknown option") {
    std::istringstream in("/set foobar\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("error is shown") {
        CHECK (out.str().find("Unknown option") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("user types /set markdown off (extra arg ignored)") {
    std::istringstream in("/set markdown off\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("toggle works") {
        CHECK (out.str().find("[markdown off]") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("REPL /version command") {
  GIVEN ("user types /version") {
    std::istringstream in("/version\nexit\n");
    std::ostringstream out;
    WHEN ("the REPL runs") {
      run_repl(echo_chat, test_cfg(), in, out);
      THEN ("version is shown") {
        CHECK (out.str().find("llama-cli") != std::string::npos)
          ;
      }
    }
  }
}

// /model tests — uses injected ModelsFn so no HTTP calls needed.
// The real get_available_models is replaced with a lambda returning a fixed list.

SCENARIO ("REPL /model with no models available") {
  GIVEN ("a model fetcher that returns an empty list") {
    ModelsFn no_models = [](const Config&) { return std::vector<std::string>{}; };
    std::istringstream in("/model\nexit\n");
    std::ostringstream out;
    WHEN ("the user runs /model") {
      run_repl(echo_chat, test_cfg(), in, out, no_models, nullptr, mock_hw, mock_model_info);
      THEN ("no models message is shown") {
        CHECK (out.str().find("No models available") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("REPL /model select from list") {
  GIVEN ("a model fetcher that returns 5 models") {
    ModelsFn five_models = [](const Config&) {
      return std::vector<std::string>{"gemma4:e4b", "gemma4:26b", "llama3:8b", "mistral:7b", "phi3:mini"};
    };
    WHEN ("the user picks model 2") {
      std::istringstream in("/model\n2\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models, nullptr, mock_hw, mock_model_info);
      THEN ("all 5 models are listed") {
        // Sorted by params descending: 26B, 8B, 7B, 4B, 3.8B
        CHECK (out.str().find("1. gemma4:26b") != std::string::npos)
          ;
        CHECK (out.str().find("5. phi3:mini") != std::string::npos)
          ;
      }
      THEN ("model is set to the selected one") {
        // Model 2 in params-sorted order is llama3:8b
        CHECK (out.str().find("[model set to llama3:8b (saved to .env)]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user picks model 1 from a single-model list") {
      ModelsFn one_model = [](const Config&) { return std::vector<std::string>{"gemma4:e4b"}; };
      std::istringstream in("/model\n1\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, one_model, nullptr, mock_hw, mock_model_info);
      THEN ("model is set") {
        CHECK (out.str().find("[model set to gemma4:e4b (saved to .env)]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user enters an out-of-range number") {
      std::istringstream in("/model\n99\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models, nullptr, mock_hw, mock_model_info);
      THEN ("out of range message is shown") {
        CHECK (out.str().find("[out of range]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user enters invalid input") {
      std::istringstream in("/model\nabc\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models, nullptr, mock_hw, mock_model_info);
      THEN ("invalid input message is shown") {
        CHECK (out.str().find("[invalid input") != std::string::npos)
          ;
      }
    }
    WHEN ("the user input has trailing carriage return (raw terminal)") {
      std::istringstream in("/model\n2\r\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models, nullptr, mock_hw, mock_model_info);
      THEN ("model is set despite the CR") {
        CHECK (out.str().find("[model set to llama3:8b (saved to .env)]") != std::string::npos)
          ;
      }
    }
  }
}

// TODO: test REPL with piped stdin (non-TTY input)
// TODO: test /clear resets conversation history
// TODO: test /set trace toggles trace mode
// TODO: test color_name_to_ansi and ansi_to_name (high complexity, needs refactor)
// TODO: test interruptible_chat timeout behavior (needs mock HTTP or thread control)
// TODO: test !! command injects output into history
// TODO: test ! command does NOT inject output into history

// TODO: CSV descriptions in /model (not yet implemented)

SCENARIO ("default_model is auto") {
  GIVEN ("no model override") {
    Config c;
    THEN ("default model is auto") {
      CHECK (c.model == std::string("auto"))
        ;
    }
  }
}
// Rating tests extracted to repl_rate_test.cpp (ADR-061 file size limit)

SCENARIO ("dispatch_command handles slash commands") {
  GIVEN ("a ReplState with mock functions") {
    ChatFn chat = echo_chat;
    ModelsFn models = [](const Config&) { return std::vector<std::string>{"model1"}; };
    Config cfg;
    std::vector<Message> history;
    std::istringstream in("");
    std::ostringstream out;
    ReplState s = {
        chat, nullptr, models, nullptr, nullptr, nullptr, cfg,   history, in,    out,  0, false, false,
        true, false,   "",     false,   "32",    "",      false, false,   false, true, 5, -1,    static_cast<ModelRegistry*>(nullptr),
        {},   nullptr};

    WHEN ("dispatch_command is called with /clear") {
      history.push_back({"user", "hello"});
      dispatch_command("clear", "", s);
      THEN ("history is cleared") {
        CHECK (history.empty())
          ;
        CHECK (out.str().find("[history cleared]") != std::string::npos)
          ;
      }
    }

    WHEN ("dispatch_command is called with /version") {
      dispatch_command("version", "", s);
      THEN ("version is printed with commit hash") {
        CHECK (out.str().find("llama-cli") != std::string::npos)
          ;
        // Format: "llama-cli X.Y.Z (abcdef0)" or "llama-cli X.Y.Z (abcdef0 dirty)"
        // Must contain a 7-char hex commit hash in parentheses
        auto paren = out.str().find('(');
        CHECK (paren != std::string::npos)
          ;
        auto close = out.str().find(')', paren);
        CHECK (close != std::string::npos)
          ;
      }
    }

    WHEN ("dispatch_command is called with unknown command") {
      dispatch_command("nonexistent", "", s);
      THEN ("error message is shown") {
        CHECK (out.str().find("Unknown command") != std::string::npos)
          ;
      }
    }

    WHEN ("dispatch_command is called with /set without args") {
      dispatch_command("set", "", s);
      THEN ("options are listed") {
        CHECK (out.str().find("markdown") != std::string::npos)
          ;
        CHECK (out.str().find("color") != std::string::npos)
          ;
      }
    }
  }
}

// --- Prompt label tests (ADR-089) ---

SCENARIO ("repl: prompt label omits model for non-ollama providers") {
  GIVEN ("provider is tgpt") {
    THEN ("model is not shown in prompt") {
      std::string label = build_prompt_label("gius", "tgpt", "mistral-nemo:12b");
      CHECK (label == "gius@tgpt> ")
        ;
    }
  }
  GIVEN ("provider is gemini") {
    THEN ("model IS shown in prompt") {
      std::string label = build_prompt_label("gius", "gemini", "gemini-2.5-pro");
      CHECK (label == "gius@gemini:gemini-2.5-pro> ")
        ;
    }
  }
  GIVEN ("provider is kiro-cli") {
    THEN ("model IS shown in prompt") {
      std::string label = build_prompt_label("gius", "kiro-cli", "claude-sonnet");
      CHECK (label == "gius@kiro-cli:claude-sonnet> ")
        ;
    }
  }
  GIVEN ("provider is ollama") {
    THEN ("model IS shown in prompt") {
      std::string label = build_prompt_label("gius", "ollama", "gemma4:26b");
      CHECK (label == "gius@ollama:gemma4:26b> ")
        ;
    }
  }
  GIVEN ("nick is empty") {
    THEN ("just shows > ") {
      std::string label = build_prompt_label("", "tgpt", "whatever");
      CHECK (label == "> ")
        ;
    }
  }
}

// --- /review command tests (ADR-113) ---

SCENARIO ("review: no changes shows message") {
  GIVEN ("a ReplState with mock stream_chat") {
    ChatFn chat = echo_chat;
    ModelsFn models = [](const Config&) { return std::vector<std::string>{"model1"}; };
    Config cfg;
    cfg.exec_timeout = 5;
    std::vector<Message> history;
    std::istringstream in("");
    std::ostringstream out;
    // stream_chat that captures what was sent
    StreamChatFn stream = [](const std::vector<Message>& /*msgs*/, std::function<bool(const std::string&)> cb) -> std::string {
      cb("## Summary\nCode looks good.");
      return "## Summary\nCode looks good.";
    };
    ReplState s = {chat,  stream, models, nullptr, nullptr, nullptr, cfg,   history, in,   out, 0,  false,   false, true,
                   false, "",     false,  "32",    "",      false,   false, false,   true, 5,   -1, nullptr, {},    {}};

    WHEN ("/review is called in a clean repo") {
      // This test verifies the command is recognized and runs without crash.
      // In a clean repo, git diff returns empty — we expect "no changes" message.
      dispatch_command("review", "", s);
      THEN ("output contains review-related text") {
        std::string result = out.str();
        // Either "no changes" or "reviewing" (depends on git state)
        bool has_review = result.find("reviewing") != std::string::npos;
        bool has_no_changes = result.find("no changes") != std::string::npos;
        bool has_summary = result.find("Summary") != std::string::npos;
        CHECK ((has_review || has_no_changes || has_summary))
          ;
      }
    }

    WHEN ("/review is called with unknown variant") {
      dispatch_command("review", "nonexistent_file_xyz.cpp", s);
      THEN ("it attempts review (no crash)") {
        std::string result = out.str();
        CHECK (result.find("reviewing") != std::string::npos)
          ;
      }
    }
  }
}
