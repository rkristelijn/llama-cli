// test_annotation.cpp — Unit tests for LLM annotation parsing

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "annotation.h"

SCENARIO("parsing write annotations") {
  GIVEN("a response with a single write annotation") {
    std::string text = R"(Here's the fix:
<write file="src/main.cpp">int main() {
  return 0;
}
</write>
Done!)";

    WHEN("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN("one action is found") {
        CHECK(actions.size() == 1);
        CHECK(actions[0].path == "src/main.cpp");
        CHECK(actions[0].content.find("int main()") != std::string::npos);
      }
    }

    WHEN("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN("the tag is replaced with a summary") {
        CHECK(clean.find("<write") == std::string::npos);
        CHECK(clean.find("[proposed: write src/main.cpp]") != std::string::npos);
        CHECK(clean.find("Done!") != std::string::npos);
      }
    }
  }

  GIVEN("a response with multiple write annotations") {
    std::string text = R"(<write file="a.h">header</write>
and
<write file="b.cpp">impl</write>)";

    WHEN("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN("both are found") {
        CHECK(actions.size() == 2);
        CHECK(actions[0].path == "a.h");
        CHECK(actions[0].content == "header");
        CHECK(actions[1].path == "b.cpp");
        CHECK(actions[1].content == "impl");
      }
    }
  }

  GIVEN("a response with no annotations") {
    std::string text = "Just a normal response.";

    WHEN("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN("none are found") { CHECK(actions.empty()); }
    }

    WHEN("annotations are stripped") {
      auto clean = strip_annotations(text);
      THEN("text is unchanged") { CHECK(clean == text); }
    }
  }

  GIVEN("a malformed annotation") {
    std::string text = R"(<write file="test.cpp">content without closing tag)";

    WHEN("annotations are parsed") {
      auto actions = parse_write_annotations(text);
      THEN("it is ignored") { CHECK(actions.empty()); }
    }
  }
}
