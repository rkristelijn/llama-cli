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
