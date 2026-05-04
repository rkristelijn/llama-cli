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

// --- Mutation-killing tests --------------------------------------------------
// These tests target surviving mutants found by mull mutation testing.
// Each test verifies exact output or multi-annotation parsing to catch
// off-by-one errors in substr/replace offset math.

// GROUP 1: Parse functions — verify content is EXACTLY correct
// (kills opening_tag substr mutants: tag_end - start + 1)

SCENARIO ("write annotation content is exactly preserved") {
  GIVEN ("a write with precise content boundaries") {
    std::string text = "<write file=\"f.cpp\">ABC</write>";
    auto actions = parse_write_annotations(text);
    THEN ("content is exactly ABC with no extra chars") {
      REQUIRE (actions.size() == 1)
        ;
      CHECK (actions[0].content == "ABC")
        ;
    }
  }
}

SCENARIO ("str_replace content boundaries are exact") {
  GIVEN ("a str_replace with single-char old and new") {
    std::string text = "<str_replace path=\"x\"><old>A</old><new>B</new></str_replace>";
    auto actions = parse_str_replace_annotations(text);
    THEN ("old and new are exactly one char each") {
      REQUIRE (actions.size() == 1)
        ;
      CHECK (actions[0].old_str == "A")
        ;
      CHECK (actions[0].new_str == "B")
        ;
    }
  }
}

SCENARIO ("multiple read annotations are all parsed") {
  GIVEN ("two read annotations in sequence") {
    std::string text = R"(<read path="a.cpp" lines="1-5"/> then <read path="b.cpp" search="foo"/>)";
    auto actions = parse_read_annotations(text);
    THEN ("both are found with correct attributes") {
      REQUIRE (actions.size() == 2)
        ;
      CHECK (actions[0].path == "a.cpp")
        ;
      CHECK (actions[0].from_line == 1)
        ;
      CHECK (actions[0].to_line == 5)
        ;
      CHECK (actions[1].path == "b.cpp")
        ;
      CHECK (actions[1].search == "foo")
        ;
    }
  }
}

SCENARIO ("multiple add_line annotations are all parsed") {
  GIVEN ("two add_line annotations") {
    std::string text = R"(<add_line path="a.txt" line_number="1" content="first"/>
<add_line path="b.txt" line_number="2" content="second"/>)";
    auto actions = parse_add_line_annotations(text);
    THEN ("both are found") {
      REQUIRE (actions.size() == 2)
        ;
      CHECK (actions[0].path == "a.txt")
        ;
      CHECK (actions[0].content == "first")
        ;
      CHECK (actions[1].path == "b.txt")
        ;
      CHECK (actions[1].content == "second")
        ;
    }
  }
}

SCENARIO ("multiple delete_line annotations are all parsed") {
  GIVEN ("two delete_line annotations") {
    std::string text = R"(<delete_line path="a.txt" content="line1"/>
<delete_line path="b.txt" content="line2"/>)";
    auto actions = parse_delete_line_annotations(text);
    THEN ("both are found") {
      REQUIRE (actions.size() == 2)
        ;
      CHECK (actions[0].path == "a.txt")
        ;
      CHECK (actions[0].content == "line1")
        ;
      CHECK (actions[1].path == "b.txt")
        ;
      CHECK (actions[1].content == "line2")
        ;
    }
  }
}

SCENARIO ("multiple search annotations are all parsed") {
  GIVEN ("two search annotations") {
    std::string text = R"(<search>first query</search> and <search>second query</search>)";
    auto actions = parse_search_annotations(text);
    THEN ("both are found") {
      REQUIRE (actions.size() == 2)
        ;
      CHECK (actions[0].query == "first query")
        ;
      CHECK (actions[1].query == "second query")
        ;
    }
  }
}

// GROUP 2: strip_annotations — exact string equality to kill offset math mutants

SCENARIO ("strip_annotations produces exact output for write") {
  GIVEN ("a write annotation with surrounding text") {
    std::string text = "before<write file=\"f.cpp\">code</write>after";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary with no leftover tag chars") {
      CHECK (clean == "before\033[1;37m[proposed: write f.cpp]\033[0mafter")
        ;
    }
  }
}

SCENARIO ("strip_annotations produces exact output for str_replace") {
  GIVEN ("a str_replace annotation with surrounding text") {
    std::string text = "A<str_replace path=\"x.cpp\"><old>a</old><new>b</new></str_replace>B";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary") {
      CHECK (clean == "A\033[1;37m[proposed: str_replace x.cpp]\033[0mB")
        ;
    }
  }
}

SCENARIO ("strip_annotations produces exact output for read") {
  GIVEN ("a read annotation with surrounding text") {
    std::string text = "X<read path=\"r.cpp\" lines=\"1-2\"/>Y";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary") {
      CHECK (clean == "X\033[1;37m[read r.cpp]\033[0mY")
        ;
    }
  }
}

SCENARIO ("strip_annotations produces exact output for search") {
  GIVEN ("a search annotation with surrounding text") {
    std::string text = "P<search>my query</search>Q";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary") {
      CHECK (clean == "P\033[1;37m[search: my query]\033[0mQ")
        ;
    }
  }
}

SCENARIO ("strip_annotations produces exact output for add_line") {
  GIVEN ("an add_line annotation with surrounding text") {
    std::string text = "M<add_line path=\"t.txt\" line_number=\"1\" content=\"x\"/>N";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary") {
      CHECK (clean == "M\033[1;37m[proposed: add line to t.txt]\033[0mN")
        ;
    }
  }
}

SCENARIO ("strip_annotations produces exact output for delete_line") {
  GIVEN ("a delete_line annotation with surrounding text") {
    std::string text = "J<delete_line path=\"d.txt\" content=\"bye\"/>K";
    auto clean = strip_annotations(text);
    THEN ("output is exactly the summary") {
      CHECK (clean == "J\033[1;37m[proposed: delete line from d.txt]\033[0mK")
        ;
    }
  }
}

// GROUP 3: fix_malformed_tags — targeted scenarios for inner-tag detection

SCENARIO ("fix_malformed_tags repairs str_replace with wrong closer") {
  GIVEN ("a str_replace closed with </replace>") {
    std::string text = R"(<str_replace path="a.cpp">content</replace>)";
    auto fixed = fix_malformed_tags(text);
    THEN ("the closer becomes </str_replace>") {
      CHECK (fixed == R"(<str_replace path="a.cpp">content</str_replace>)")
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags handles multiple malformed tags in sequence") {
  GIVEN ("two exec tags both with wrong closers") {
    std::string text = "<exec>cmd1</bash><exec>cmd2</sh>";
    auto fixed = fix_malformed_tags(text);
    THEN ("both are repaired") {
      CHECK (fixed == "<exec>cmd1</exec><exec>cmd2</exec>")
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags skips self-closing tags") {
  GIVEN ("add_line and delete_line mixed with exec") {
    std::string text = R"(<add_line path="a" line_number="1" content="x"/><exec>ls</wrong>)";
    auto fixed = fix_malformed_tags(text);
    THEN ("self-closing tags are untouched and exec is repaired") {
      CHECK (fixed.find("<add_line") != std::string::npos)
        ;
      CHECK (fixed.find("</exec>") != std::string::npos)
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags preserves valid inner closers") {
  GIVEN ("a search tag closed with wrong closer after valid exec block") {
    std::string text = "<exec>ls</exec><search>query</wrong>";
    auto fixed = fix_malformed_tags(text);
    THEN ("exec is untouched and search closer is fixed") {
      CHECK (fixed == "<exec>ls</exec><search>query</search>")
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags handles search with wrong closer") {
  GIVEN ("a search tag closed with </query>") {
    std::string text = "<search>find stuff</query>";
    auto fixed = fix_malformed_tags(text);
    THEN ("closer becomes </search>") {
      CHECK (fixed == "<search>find stuff</search>")
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags: correct closer before next opener wins") {
  GIVEN ("two write tags where first has correct closer") {
    std::string text = R"(<write file="a">x</write><write file="b">y</write>)";
    auto fixed = fix_malformed_tags(text);
    THEN ("both are preserved unchanged") {
      CHECK (fixed == text)
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags: wrong closer between two openers") {
  GIVEN ("first write has wrong closer, second is correct") {
    std::string text = R"(<write file="a">x</wrong><write file="b">y</write>)";
    auto fixed = fix_malformed_tags(text);
    THEN ("first closer is fixed, second is untouched") {
      CHECK (fixed == R"(<write file="a">x</write><write file="b">y</write>)")
        ;
    }
  }
}

SCENARIO ("fix_malformed_tags: inner write closer inside exec is skipped") {
  GIVEN ("exec containing a write with its own correct closer") {
    std::string text = "<exec><write file=\"a\">data</write></bash>";
    auto fixed = fix_malformed_tags(text);
    THEN ("write closer is preserved and exec gets correct closer") {
      CHECK (fixed.find("</write>") != std::string::npos)
        ;
      CHECK (fixed.find("</exec>") != std::string::npos)
        ;
    }
  }
}
