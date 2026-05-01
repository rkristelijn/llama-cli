// test_annotation.cpp — Unit tests for LLM annotation parsing

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "annotation/annotation.h"

#include <doctest/doctest.h>

SCENARIO ("parsing write annotations") {
  GIVEN ("a response with a single write annotation") {
    std::string text = R"(Here's the fix:
<write file="src/main.cpp">int main() {
  return 0;
}
</write>
Done!)";

    WHEN ("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN ("one action is found") {
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].path == "src/main.cpp")
          ;
        CHECK (actions[0].content.find("int main()") != std::string::npos)
          ;
      }
    }

    WHEN ("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN ("the tag is replaced with a summary") {
        CHECK (clean.find("<write") == std::string::npos)
          ;
        CHECK (clean.find("[proposed: write src/main.cpp]") != std::string::npos)
          ;
        CHECK (clean.find("Done!") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("a response with multiple write annotations") {
    std::string text = R"(<write file="a.h">header</write>
and
<write file="b.cpp">impl</write>)";

    WHEN ("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN ("both are found") {
        CHECK (actions.size() == 2)
          ;
        CHECK (actions[0].path == "a.h")
          ;
        CHECK (actions[0].content == "header")
          ;
        CHECK (actions[1].path == "b.cpp")
          ;
        CHECK (actions[1].content == "impl")
          ;
      }
    }
  }

  GIVEN ("a response with no annotations") {
    std::string text = "Just a normal response.";

    WHEN ("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN ("none are found") {
        CHECK (actions.empty())
          ;
      }
    }

    WHEN ("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN ("text is unchanged") {
        CHECK (clean == text)
          ;
      }
    }
  }

  GIVEN ("a malformed annotation") {
    std::string text = R"(<write file="test.cpp">content without closing tag)";

    WHEN ("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN ("it is ignored") {
        CHECK (actions.empty())
          ;
      }
    }
  }
}

SCENARIO ("parsing str_replace annotations") {
  GIVEN ("a response with a str_replace annotation") {
    std::string text = R"(Fix:
<str_replace path="src/main.cpp">
<old>int x = 1;</old>
<new>int x = 2;</new>
</str_replace>
Done.)";

    WHEN ("annotations are parsed") {
      auto actions = parse_str_replace_annotations(text);
      THEN ("one action is found") {
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].path == "src/main.cpp")
          ;
        CHECK (actions[0].old_str == "int x = 1;")
          ;
        CHECK (actions[0].new_str == "int x = 2;")
          ;
      }
    }

    WHEN ("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN ("tag is replaced with summary") {
        CHECK (clean.find("<str_replace") == std::string::npos)
          ;
        CHECK (clean.find("[proposed: str_replace src/main.cpp]") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("parsing read annotations") {
  GIVEN ("a read annotation with line range") {
    std::string text = R"(Let me check: <read path="src/main.cpp" lines="10-20"/>)";

    WHEN ("annotations are parsed") {
      auto actions = parse_read_annotations(text);
      THEN ("one action is found") {
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].path == "src/main.cpp")
          ;
        CHECK (actions[0].from_line == 10)
          ;
        CHECK (actions[0].to_line == 20)
          ;
        CHECK (actions[0].search.empty())
          ;
      }
    }

    WHEN ("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN ("tag is replaced with summary") {
        CHECK (clean.find("<read") == std::string::npos)
          ;
        CHECK (clean.find("[read src/main.cpp]") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("a read annotation with search term") {
    std::string text = R"(<read path="src/repl.cpp" search="confirm_write"/>)";

    WHEN ("annotations are parsed") {
      auto actions = parse_read_annotations(text);
      THEN ("search term is extracted") {
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].search == "confirm_write")
          ;
        CHECK (actions[0].from_line == 0)
          ;
      }
    }
  }

  GIVEN ("a read annotation with malformed line range") {
    std::string text = R"(<read path="src/main.cpp" lines="x-y"/>)";

    WHEN ("annotations are parsed") {
      auto actions = parse_read_annotations(text);
      THEN ("it does not crash and defaults to 0") {
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].from_line == 0)
          ;
        CHECK (actions[0].to_line == 0)
          ;
      }
    }
  }
}

SCENARIO ("stripped annotations contain bold white ANSI codes") {
  GIVEN ("a write annotation") {
    std::string text = R"(<write file="test.cpp">content</write>)";
    WHEN ("stripped") {
      auto clean = strip_annotations(text);
      THEN ("output contains bold white escape and reset") {
        CHECK (clean.find("\033[1;37m") != std::string::npos)
          ;
        CHECK (clean.find("\033[0m") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("a str_replace annotation") {
    std::string text = R"(<str_replace path="x.cpp"><old>a</old><new>b</new></str_replace>)";
    WHEN ("stripped") {
      auto clean = strip_annotations(text);
      THEN ("output contains bold white escape") {
        CHECK (clean.find("\033[1;37m") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("a read annotation") {
    std::string text = R"(<read path="x.cpp" lines="1-5"/>)";
    WHEN ("stripped") {
      auto clean = strip_annotations(text);
      THEN ("output contains bold white escape") {
        CHECK (clean.find("\033[1;37m") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("fix_malformed_tags repairs broken closing tags") {
  GIVEN ("an exec tag closed with </bash>") {
    std::string text = "<exec>echo hello</bash>";
    WHEN ("fixed") {
      auto fixed = fix_malformed_tags(text);
      THEN ("closing tag becomes </exec>") {
        CHECK (fixed == "<exec>echo hello</exec>")
          ;
      }
    }
  }

  GIVEN ("a write tag closed with </file>") {
    std::string text = R"(<write file="a.txt">content</file>)";
    WHEN ("fixed") {
      auto fixed = fix_malformed_tags(text);
      THEN ("closing tag becomes </write>") {
        CHECK (fixed.find("</write>") != std::string::npos)
          ;
      }
      THEN ("write parser can extract it") {
        auto actions = parse_write_annotations(fixed);
        CHECK (actions.size() == 1)
          ;
        CHECK (actions[0].path == "a.txt")
          ;
      }
    }
  }

  GIVEN ("well-formed tags") {
    std::string text = "<exec>ls</exec> and <search>query</search>";
    WHEN ("fixed") {
      auto fixed = fix_malformed_tags(text);
      THEN ("text is unchanged") {
        CHECK (fixed == text)
          ;
      }
    }
  }

  GIVEN ("no tags at all") {
    std::string text = "just plain text";
    WHEN ("fixed") {
      auto fixed = fix_malformed_tags(text);
      THEN ("text is unchanged") {
        CHECK (fixed == text)
          ;
      }
    }
  }
}

SCENARIO ("parsing search annotations") {
  GIVEN ("a search annotation") {
    std::string text = R"(<search>how to parse JSON in C++</search>)";
    auto actions = parse_search_annotations(text);
    THEN ("query is extracted") {
      REQUIRE (actions.size() == 1)
        ;
      CHECK (actions[0].query == "how to parse JSON in C++")
        ;
    }
  }
  GIVEN ("no search annotations") {
    auto actions = parse_search_annotations("just plain text");
    THEN ("result is empty") {
      CHECK (actions.empty())
        ;
    }
  }
}

SCENARIO ("parsing add_line annotations") {
  GIVEN ("an add_line annotation") {
    std::string text = R"(<add_line path="test.txt" line_number="3" content="new line content"/>)";
    auto actions = parse_add_line_annotations(text);
    THEN ("action is extracted") {
      REQUIRE (actions.size() == 1)
        ;
      CHECK (actions[0].path == "test.txt")
        ;
      CHECK (actions[0].line_number == 3)
        ;
      CHECK (actions[0].content == "new line content")
        ;
    }
  }
}

SCENARIO ("parsing delete_line annotations") {
  GIVEN ("a delete_line annotation") {
    std::string text = R"(<delete_line path="test.txt" content="old line"/>)";
    auto actions = parse_delete_line_annotations(text);
    THEN ("action is extracted") {
      REQUIRE (actions.size() == 1)
        ;
      CHECK (actions[0].path == "test.txt")
        ;
      CHECK (actions[0].content == "old line")
        ;
    }
  }
}
