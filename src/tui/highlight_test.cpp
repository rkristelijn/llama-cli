/**
 * @file highlight_test.cpp
 * @brief Unit tests for pluggable syntax highlighting.
 *
 * Tests: language detection, keyword highlighting, strings, comments,
 * numbers, preprocessor, unknown languages, and the pluggable interface.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tui/highlight.h"

#include <doctest/doctest.h>

#include <string>

using tui::RegexHighlighter;

// Helper: check if output contains an ANSI escape sequence
static bool has_ansi(const std::string& s) { return s.find("\033[") != std::string::npos; }

// Helper: strip ANSI codes to get plain text
static std::string strip_ansi(const std::string& s) {
  std::string out;
  size_t i = 0;
  while (i < s.size()) {
    if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
      // Skip until 'm'
      while (i < s.size() && s[i] != 'm') {
        ++i;
      }
      ++i;
    } else {
      out += s[i];
      ++i;
    }
  }
  return out;
}

SCENARIO ("highlight: unknown language gets base color only") {
  GIVEN ("a line of code with unknown language") {
    RegexHighlighter h;
    WHEN ("highlighting with empty lang") {
      auto result = h.highlight_line("x = 1", "");
      THEN ("output has ANSI but plain text is preserved") {
        CHECK (has_ansi(result))
          ;
        CHECK (strip_ansi(result) == "x = 1")
          ;
      }
    }
    WHEN ("highlighting with unknown lang") {
      auto result = h.highlight_line("foo bar", "brainfuck");
      THEN ("plain text is preserved") {
        CHECK (strip_ansi(result) == "foo bar")
          ;
      }
    }
  }
}

SCENARIO ("highlight: C++ keywords are colored") {
  GIVEN ("a line with C++ keywords") {
    RegexHighlighter h;
    WHEN ("highlighting 'return 0;'") {
      auto result = h.highlight_line("return 0;", "cpp");
      THEN ("output contains bold yellow for keyword") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "return 0;")
          ;
      }
    }
    WHEN ("highlighting 'if (x > 0)'") {
      auto result = h.highlight_line("if (x > 0)", "cpp");
      THEN ("'if' is highlighted as keyword") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "if (x > 0)")
          ;
      }
    }
  }
}

SCENARIO ("highlight: strings are colored green") {
  GIVEN ("a line with a string literal") {
    RegexHighlighter h;
    WHEN ("highlighting a double-quoted string") {
      auto result = h.highlight_line("msg = \"hello\"", "python");
      THEN ("string is green") {
        CHECK (result.find("\033[32m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "msg = \"hello\"")
          ;
      }
    }
    WHEN ("highlighting a single-quoted string") {
      auto result = h.highlight_line("c = 'x'", "python");
      THEN ("string is green") {
        CHECK (result.find("\033[32m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "c = 'x'")
          ;
      }
    }
  }
}

SCENARIO ("highlight: comments are dim gray") {
  GIVEN ("a line with a comment") {
    RegexHighlighter h;
    WHEN ("highlighting a C++ comment") {
      auto result = h.highlight_line("// this is a comment", "cpp");
      THEN ("comment is dim gray") {
        CHECK (result.find("\033[2;37m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "// this is a comment")
          ;
      }
    }
    WHEN ("highlighting a Python comment") {
      auto result = h.highlight_line("# this is a comment", "python");
      THEN ("comment is dim gray") {
        CHECK (result.find("\033[2;37m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "# this is a comment")
          ;
      }
    }
  }
}

SCENARIO ("highlight: numbers are magenta") {
  GIVEN ("a line with numeric literals") {
    RegexHighlighter h;
    WHEN ("highlighting 'x = 42'") {
      auto result = h.highlight_line("x = 42", "cpp");
      THEN ("number is magenta") {
        CHECK (result.find("\033[35m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "x = 42")
          ;
      }
    }
  }
}

SCENARIO ("highlight: preprocessor directives") {
  GIVEN ("a C++ preprocessor line") {
    RegexHighlighter h;
    WHEN ("highlighting '#include <iostream>'") {
      auto result = h.highlight_line("#include <iostream>", "cpp");
      THEN ("line is bold magenta") {
        CHECK (result.find("\033[1;35m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "#include <iostream>")
          ;
      }
    }
  }
}

SCENARIO ("highlight: Python keywords") {
  GIVEN ("a Python line") {
    RegexHighlighter h;
    WHEN ("highlighting 'def foo():'") {
      auto result = h.highlight_line("def foo():", "python");
      THEN ("'def' is highlighted") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "def foo():")
          ;
      }
    }
  }
}

SCENARIO ("highlight: Bash keywords") {
  GIVEN ("a bash line") {
    RegexHighlighter h;
    WHEN ("highlighting 'if [ -f file ]; then'") {
      auto result = h.highlight_line("if [ -f file ]; then", "bash");
      THEN ("'if' and 'then' are highlighted") {
        CHECK (strip_ansi(result) == "if [ -f file ]; then")
          ;
        // Both keywords should trigger bold yellow
        auto first = result.find("\033[1;33m");
        auto second = result.find("\033[1;33m", first + 1);
        CHECK (first != std::string::npos)
          ;
        CHECK (second != std::string::npos)
          ;
      }
    }
  }
}
SCENARIO ("highlight: Java keywords") {
  GIVEN ("a Java line") {
    RegexHighlighter h;
    WHEN ("highlighting 'public static void main'") {
      auto result = h.highlight_line("public static void main(String[] args) {", "java");
      THEN ("keywords are highlighted") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "public static void main(String[] args) {")
          ;
      }
    }
  }
}

SCENARIO ("highlight: PHP keywords") {
  GIVEN ("a PHP line") {
    RegexHighlighter h;
    WHEN ("highlighting 'function hello()'") {
      auto result = h.highlight_line("function hello() {", "php");
      THEN ("'function' is highlighted") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "function hello() {")
          ;
      }
    }
  }
}

SCENARIO ("highlight: Swift keywords") {
  GIVEN ("a Swift line") {
    RegexHighlighter h;
    WHEN ("highlighting 'func hello()'") {
      auto result = h.highlight_line("func hello() {", "swift");
      THEN ("'func' is highlighted") {
        CHECK (result.find("\033[1;33m") != std::string::npos)
          ;
        CHECK (strip_ansi(result) == "func hello() {")
          ;
      }
    }
  }
}

SCENARIO ("highlight: active_highlighter returns a valid instance") {
  GIVEN ("the global highlighter") {
    WHEN ("accessing active_highlighter") {
      const auto& h = tui::active_highlighter();
      THEN ("it has a name") {
        CHECK (!h.name().empty())
          ;
      }
      THEN ("it can highlight a line") {
        auto result = h.highlight_line("int x = 1;", "cpp");
        CHECK (!result.empty())
          ;
        CHECK (strip_ansi(result) == "int x = 1;")
          ;
      }
    }
  }
}
