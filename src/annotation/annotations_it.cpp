/**
 * @file annotations_it.cpp
 * @brief Integration tests: write y/n/s, exec y/n
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <fstream>
#include <sstream>

#include "test_helpers.h"

SCENARIO ("Write annotation with confirm and skip") {
  const std::string path = "/tmp/llama-int-write.txt";
  std::remove(path.c_str());

  GIVEN ("the LLM responds with a write annotation") {
    auto write_chat = [&](const std::vector<Message>&) { return "Here: <write file=\"" + path + "\">integration test</write>"; };

    WHEN ("the user confirms with y") {
      std::istringstream in("write it\ny\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("the file is written") {
        std::ifstream f(path);
        CHECK (f.is_open())
          ;
        std::string content;
        std::getline(f, content);
        CHECK (content == "integration test")
          ;
      }
    }

    WHEN ("the user declines with n") {
      std::remove(path.c_str());
      std::istringstream in("write it\nn\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("the file is not written") {
        std::ifstream f(path);
        CHECK_FALSE (f.is_open())
          ;
      }
      THEN ("[skipped] is shown") {
        CHECK (out.str().find("[skipped]") != std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
}

SCENARIO ("Write annotation with show then confirm") {
  const std::string path = "/tmp/llama-int-show.txt";
  std::remove(path.c_str());

  GIVEN ("the LLM responds with a write annotation") {
    auto write_chat = [&](const std::vector<Message>&) { return "Here: <write file=\"" + path + "\">show content</write>"; };
    WHEN ("the user types s then y") {
      std::istringstream in("write it\ns\ny\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);
      THEN ("content is previewed") {
        CHECK (out.str().find("show content") != std::string::npos)
          ;
      }
      THEN ("file is written") {
        std::ifstream f(path);
        CHECK (f.is_open())
          ;
      }
    }
  }
  std::remove(path.c_str());
}

SCENARIO ("Exec annotation with confirm and skip") {
  GIVEN ("the LLM responds with an exec annotation") {
    int call_count = 0;
    auto exec_chat = [&](const std::vector<Message>& /*msgs*/) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "Let me check: <exec>echo integration123</exec>";
      }
      return "I see the output";
    };

    WHEN ("the user confirms with y") {
      std::istringstream in("do it\ny\nexit\n");
      std::ostringstream out;
      run_repl(exec_chat, test_cfg(), in, out);
      THEN ("command output is shown") {
        CHECK (out.str().find("integration123") != std::string::npos)
          ;
      }
      THEN ("LLM gets a follow-up") {
        CHECK (call_count == 2)
          ;
      }
    }

    WHEN ("the user declines with n") {
      call_count = 0;  // reset shared counter between WHEN blocks
      std::istringstream in("do it\nn\nexit\n");
      std::ostringstream out;
      run_repl(exec_chat, test_cfg(), in, out);
      THEN ("[skipped] is shown") {
        CHECK (out.str().find("[skipped]") != std::string::npos)
          ;
      }
      // cppcheck-suppress knownConditionTrueFalse
      THEN ("no follow-up call") {
        CHECK (call_count == 1)
          ;
      }
    }
  }
}

SCENARIO ("Write annotation shows diff for existing files") {
  const std::string path = "/tmp/llama-int-diff.txt";
  // Create existing file
  {
    std::ofstream f(path);
    f << "old content\n";
  }

  GIVEN ("the LLM proposes overwriting an existing file") {
    auto write_chat = [&](const std::vector<Message>&) { return "Here: <write file=\"" + path + "\">new content</write>"; };

    WHEN ("the user confirms with y (diff shown automatically)") {
      std::istringstream in("write it\ny\nexit\n");
      std::ostringstream out;
      run_repl(write_chat, test_cfg(), in, out);

      THEN ("diff is shown automatically with - and +") {
        CHECK (out.str().find("- old content") != std::string::npos)
          ;
        CHECK (out.str().find("+ new content") != std::string::npos)
          ;
      }
      THEN ("file is written") {
        std::ifstream f(path);
        std::string content;
        std::getline(f, content);
        CHECK (content == "new content")
          ;
      }
      THEN ("backup exists") {
        std::ifstream bak(path + ".bak");
        CHECK (bak.is_open())
          ;
      }
    }
  }
  std::remove(path.c_str());
  std::remove((path + ".bak").c_str());
}

SCENARIO ("str_replace annotation applies targeted replacement") {
  const std::string path = "/tmp/llama-int-strreplace.txt";
  {
    std::ofstream f(path);
    f << "line one\nline two\nline three\n";
  }

  GIVEN ("the LLM proposes a str_replace") {
    auto chat = [&](const std::vector<Message>&) {
      return "Fix: <str_replace path=\"" + path + "\"><old>line two</old><new>line TWO</new></str_replace>";
    };

    WHEN ("the user confirms") {
      std::istringstream in("fix it\ny\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("file contains replacement") {
        std::ifstream f(path);
        std::string content((std::istreambuf_iterator<char>(f)), {});
        CHECK (content.find("line TWO") != std::string::npos)
          ;
        CHECK (content.find("line two") == std::string::npos)
          ;
      }
    }

    WHEN ("the user declines") {
      std::istringstream in("fix it\nn\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("[skipped] is shown") {
        CHECK (out.str().find("[skipped]") != std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
  std::remove((path + ".bak").c_str());
}

SCENARIO ("str_replace diff shows hunk headers and context") {
  const std::string path = "/tmp/llama-int-diff-hunk.txt";
  {
    std::ofstream f(path);
    // 10 lines: change only line 5 — diff should NOT show all 10 lines
    f << "line 1\nline 2\nline 3\nline 4\nline 5\nline 6\nline 7\nline 8\nline 9\nline 10\n";
  }

  GIVEN ("a str_replace changes one line in a large file") {
    auto chat = [&](const std::vector<Message>&) {
      return "<str_replace path=\"" + path + "\"><old>line 5</old><new>LINE FIVE</new></str_replace>";
    };

    WHEN ("the user confirms") {
      std::istringstream in("fix\ny\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("hunk header is shown") {
        CHECK (out.str().find("@@") != std::string::npos)
          ;
      }
      THEN ("changed lines are shown with -/+") {
        CHECK (out.str().find("- line 5") != std::string::npos)
          ;
        CHECK (out.str().find("+ LINE FIVE") != std::string::npos)
          ;
      }
      THEN ("nearby context is shown") {
        CHECK (out.str().find("line 4") != std::string::npos)
          ;
        CHECK (out.str().find("line 6") != std::string::npos)
          ;
      }
      THEN ("distant lines are NOT shown") {
        CHECK (out.str().find("line 1") == std::string::npos)
          ;
        CHECK (out.str().find("line 10") == std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
  std::remove((path + ".bak").c_str());
}

SCENARIO ("write diff shows hunk headers for existing file") {
  const std::string path = "/tmp/llama-int-diff-write.txt";
  {
    std::ofstream f(path);
    f << "L01\nL02\nL03\nL04\nL05\nL06\nL07\nL08\nL09\nL10\n";
  }

  GIVEN ("the LLM proposes overwriting with one line changed") {
    auto chat = [&](const std::vector<Message>&) {
      return "<write file=\"" + path + "\">L01\nL02\nL03\nL04\nL05\nL06\nNEW7\nL08\nL09\nL10</write>";
    };

    WHEN ("the user confirms") {
      std::istringstream in("write\ny\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("hunk header is present") {
        CHECK (out.str().find("@@") != std::string::npos)
          ;
      }
      THEN ("only the changed line is shown as -/+") {
        CHECK (out.str().find("- L07") != std::string::npos)
          ;
        CHECK (out.str().find("+ NEW7") != std::string::npos)
          ;
      }
      THEN ("distant unchanged lines are not shown") {
        CHECK (out.str().find("L01") == std::string::npos)
          ;
        CHECK (out.str().find("L02") == std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
  std::remove((path + ".bak").c_str());
}

SCENARIO ("read annotation injects file content into context") {
  const std::string path = "/tmp/llama-int-read.txt";
  {
    std::ofstream f(path);
    f << "alpha\nbeta\ngamma\ndelta\nepsilon\n";
  }

  GIVEN ("the LLM requests a line range") {
    int call_count = 0;
    auto chat = [&](const std::vector<Message>& msgs) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "Let me read: <read path=\"" + path + "\" lines=\"2-4\"/>";
      }
      // Second call: verify context was injected
      bool has_content = false;
      for (const auto& m : msgs) {
        if (m.content.find("beta") != std::string::npos) has_content = true;
      }
      CHECK (has_content)
        ;
      return "got it";
    };

    std::istringstream in("read file\nexit\n");
    std::ostringstream out;
    run_repl(chat, test_cfg(), in, out);
    THEN ("LLM gets a follow-up with file content") {
      CHECK (call_count == 2)
        ;
    }
    THEN ("[read ...] is shown to user") {
      CHECK (out.str().find("[read ") != std::string::npos)
        ;
    }
  }
  std::remove(path.c_str());
}

SCENARIO ("trust mode auto-approves subsequent writes") {
  const std::string p1 = "/tmp/llama-int-trust-w1.txt";
  const std::string p2 = "/tmp/llama-int-trust-w2.txt";
  std::remove(p1.c_str());
  std::remove(p2.c_str());

  GIVEN ("the LLM proposes two writes and user trusts on the first") {
    int call_count = 0;
    auto chat = [&](const std::vector<Message>&) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "<write file=\"" + p1 + "\">one</write>\n<write file=\"" + p2 + "\">two</write>";
      }
      return "done";
    };

    WHEN ("user answers t on first write") {
      // 't' for first write, no prompt expected for second
      std::istringstream in("go\nt\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("both files are written") {
        std::ifstream f1(p1);
        CHECK (f1.is_open())
          ;
        std::ifstream f2(p2);
        CHECK (f2.is_open())
          ;
      }
    }
  }
  std::remove(p1.c_str());
  std::remove(p2.c_str());
  std::remove((p1 + ".bak").c_str());
  std::remove((p2 + ".bak").c_str());
}

SCENARIO ("trust mode auto-approves subsequent str_replaces") {
  const std::string path = "/tmp/llama-int-trust-sr.txt";
  {
    std::ofstream f(path);
    f << "aaa\nbbb\nccc\n";
  }

  GIVEN ("the LLM proposes a str_replace and user trusts") {
    auto chat = [&](const std::vector<Message>&) {
      return "<str_replace path=\"" + path + "\"><old>bbb</old><new>BBB</new></str_replace>";
    };

    WHEN ("user answers t") {
      std::istringstream in("fix\nt\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("replacement is applied") {
        std::ifstream f(path);
        std::string content((std::istreambuf_iterator<char>(f)), {});
        CHECK (content.find("BBB") != std::string::npos)
          ;
      }
      THEN ("[wrote] is shown") {
        CHECK (out.str().find("[wrote") != std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
  std::remove((path + ".bak").c_str());
}

SCENARIO ("trust mode auto-approves subsequent execs") {
  GIVEN ("the LLM proposes two execs") {
    int call_count = 0;
    auto chat = [&](const std::vector<Message>&) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "<exec>echo first</exec>";
      }
      if (call_count == 2) {
        return "<exec>echo second</exec>";
      }
      return "all done";
    };

    WHEN ("user trusts on first exec") {
      std::istringstream in("go\nt\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("both commands execute") {
        CHECK (out.str().find("first") != std::string::npos)
          ;
        CHECK (out.str().find("second") != std::string::npos)
          ;
      }
      THEN ("no second prompt is shown") {
        // Only one "Run:" prompt should appear (the first one)
        auto s = out.str();
        auto first = s.find("Run:");
        auto second = s.find("Run:", first + 1);
        CHECK (second == std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("copy to clipboard for str_replace skips apply") {
  const std::string path = "/tmp/llama-int-copy-sr.txt";
  {
    std::ofstream f(path);
    f << "old line\n";
  }

  GIVEN ("the LLM proposes a str_replace") {
    auto chat = [&](const std::vector<Message>&) {
      return "<str_replace path=\"" + path + "\"><old>old line</old><new>new line</new></str_replace>";
    };

    WHEN ("user answers c") {
      std::istringstream in("fix\nc\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("file is NOT modified") {
        std::ifstream f(path);
        std::string content((std::istreambuf_iterator<char>(f)), {});
        CHECK (content.find("old line") != std::string::npos)
          ;
      }
      THEN ("[copied to clipboard] is shown") {
        CHECK (out.str().find("[copied") != std::string::npos)
          ;
      }
    }
  }
  std::remove(path.c_str());
}

SCENARIO ("followup loop continues until model stops producing annotations") {
  GIVEN ("the LLM produces exec on first two calls, then stops") {
    int call_count = 0;
    auto chat = [&](const std::vector<Message>&) -> std::string {
      call_count++;
      if (call_count == 1) {
        return "<exec>echo step1</exec>";
      }
      if (call_count == 2) {
        return "<exec>echo step2</exec>";
      }
      return "all done";
    };

    WHEN ("user trusts on first exec so followups auto-approve") {
      std::istringstream in("go\nt\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("model is called 3 times (initial + 2 followups)") {
        CHECK (call_count == 3)
          ;
      }
      THEN ("both exec outputs appear") {
        CHECK (out.str().find("step1") != std::string::npos)
          ;
        CHECK (out.str().find("step2") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("reminder nudge injected after iteration 2") {
  GIVEN ("the user sends 3 prompts") {
    int call_count = 0;
    bool saw_reminder = false;
    auto chat = [&](const std::vector<Message>& msgs) -> std::string {
      call_count++;
      // On the 3rd call (iteration 2, 0-indexed), check for reminder
      if (call_count == 3) {
        for (const auto& m : msgs) {
          if (m.role == "system" && m.content.find("Reminder:") != std::string::npos) {
            saw_reminder = true;
          }
        }
      }
      return "ok";
    };

    WHEN ("the REPL runs 3 prompts") {
      std::istringstream in("first\nsecond\nthird\nexit\n");
      std::ostringstream out;
      run_repl(chat, test_cfg(), in, out);
      THEN ("reminder is present on the 3rd call") {
        CHECK (call_count == 3)
          ;
        CHECK (saw_reminder)
          ;
      }
    }
  }
}