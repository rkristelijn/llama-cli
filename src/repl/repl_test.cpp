// test_repl.cpp — Unit tests for REPL loop
// Uses injectable chat function and string streams for I/O
// Tests: basic flow, history, commands, write y/n/s, exec !/!!/<exec>,
// /set toggles, /version

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "repl/repl.h"

#include <doctest/doctest.h>

#include <fstream>
#include <sstream>

#include "config/config.h"

// Mock chat: returns last user message prefixed with "echo: "
static std::string echo_chat(const std::vector<Message>& messages) { return "echo: " + messages.back().content; }

// Config with empty system prompt for simpler test assertions
static Config test_cfg() {
  Config c;
  c.system_prompt = "";
  return c;
}

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
        CHECK (out.str().find("Run: \033[1;33mecho test123\033[0m?") != std::string::npos)
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
      run_repl(echo_chat, test_cfg(), in, out, no_models);
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
      run_repl(echo_chat, test_cfg(), in, out, five_models);
      THEN ("all 5 models are listed") {
        CHECK (out.str().find("1. gemma4:e4b") != std::string::npos)
          ;
        CHECK (out.str().find("5. phi3:mini") != std::string::npos)
          ;
      }
      THEN ("model is set to the selected one") {
        CHECK (out.str().find("[model set to gemma4:26b]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user picks model 1 from a single-model list") {
      ModelsFn one_model = [](const Config&) { return std::vector<std::string>{"gemma4:e4b"}; };
      std::istringstream in("/model\n1\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, one_model);
      THEN ("model is set") {
        CHECK (out.str().find("[model set to gemma4:e4b]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user enters an out-of-range number") {
      std::istringstream in("/model\n99\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models);
      THEN ("out of range message is shown") {
        CHECK (out.str().find("[out of range]") != std::string::npos)
          ;
      }
    }
    WHEN ("the user enters invalid input") {
      std::istringstream in("/model\nabc\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, five_models);
      THEN ("invalid input message is shown") {
        CHECK (out.str().find("[invalid input]") != std::string::npos)
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

SCENARIO ("REPL /model shows descriptions from CSV") {
  GIVEN ("a models.csv exists with descriptions") {
    // Write a temp CSV for the test
    {
      std::ofstream csv(".cache/models.csv");
      csv << "model,description,tok_s,vram,arch\n";
      csv << "alpha:7b,fast general model,40.0,8GB,MoE\n";
      csv << "beta:13b,coding specialist,25.0,14GB,dense\n";
    }
    ModelsFn two_models = [](const Config&) { return std::vector<std::string>{"alpha:7b", "beta:13b"}; };
    WHEN ("the user runs /model and cancels") {
      std::istringstream in("/model\n1\nexit\n");
      std::ostringstream out;
      run_repl(echo_chat, test_cfg(), in, out, two_models);
      THEN ("descriptions are shown next to model names") {
        CHECK (out.str().find("fast general model") != std::string::npos)
          ;
        CHECK (out.str().find("coding specialist") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("DEFAULT_MODEL is auto") {
  GIVEN ("no model override") {
    Config c;
    THEN ("default model is auto") {
      CHECK (c.model == std::string("auto"))
        ;
    }
  }
}
// Rating prompt only shows in interactive TTY mode (not testable via stringstream).
// /rate command is testable because it's a regular slash command.

SCENARIO ("REPL /rate command") {
  GIVEN ("user rates last response via /rate") {
    std::istringstream in("hello\n/rate last +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates last response as negative") {
    std::istringstream in("hello\n/rate last -\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: negative]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user saves last response for review") {
    std::istringstream in("hello\n/rate last s\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: saved]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user lists rated responses after /rate") {
    std::istringstream in("hello\n/rate last +\n/rate list\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("list shows rated responses") {
      CHECK (out.str().find("1. [positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user lists with no ratings") {
    std::istringstream in("hello\n/rate list\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("no rated responses message is shown") {
      CHECK (out.str().find("[no rated responses]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user provides invalid /rate syntax") {
    std::istringstream in("/rate\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("usage help is shown") {
      CHECK (out.str().find("Usage: /rate") != std::string::npos)
        ;
    }
  }

  GIVEN ("user provides invalid rating value") {
    std::istringstream in("hello\n/rate last x\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("invalid rating message is shown") {
      CHECK (out.str().find("Invalid rating") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates a specific response by index") {
    std::istringstream in("first\nsecond\n/rate 1 +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("first response is rated") {
      CHECK (out.str().find("[rated: positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates with no prior response") {
    std::istringstream in("/rate last +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("response not found message is shown") {
      CHECK (out.str().find("Response not found") != std::string::npos)
        ;
    }
  }
}