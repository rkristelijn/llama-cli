// test_annotation.cpp — Unit tests for LLM annotation parsing

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "annotation/annotation.h"

#include <doctest/doctest.h>

SCENARIO("parsing write annotations")
{
  GIVEN("a response with a single write annotation")
  {
    std::string text = R"(Here's the fix:
<write file="src/main.cpp">int main() {
  return 0;
}
</write>
Done!)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_write_annotations(text);
      THEN("one action is found")
      {
        CHECK(actions.size() == 1);
        CHECK(actions[0].path == "src/main.cpp");
        CHECK(actions[0].content.find("int main()") != std::string::npos);
      }
    }

    WHEN("annotations are stripped")
    {
      auto clean = strip_annotations(text);
      THEN("the tag is replaced with a summary")
      {
        CHECK(clean.find("<write") == std::string::npos);
        CHECK(clean.find("[proposed: write src/main.cpp]") != std::string::npos);
        CHECK(clean.find("Done!") != std::string::npos);
      }
    }
  }

  GIVEN("a response with multiple write annotations")
  {
    std::string text = R"(<write file="a.h">header</write>
and
<write file="b.cpp">impl</write>)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_write_annotations(text);
      THEN("both are found")
      {
        CHECK(actions.size() == 2);
        CHECK(actions[0].path == "a.h");
        CHECK(actions[0].content == "header");
        CHECK(actions[1].path == "b.cpp");
        CHECK(actions[1].content == "impl");
      }
    }
  }

  GIVEN("a response with no annotations")
  {
    std::string text = "Just a normal response.";

    WHEN("annotations are parsed")
    {
      auto actions = parse_write_annotations(text);
      THEN("none are found")
      {
        CHECK(actions.empty());
      }
    }

    WHEN("annotations are stripped")
    {
      auto clean = strip_annotations(text);
      THEN("text is unchanged")
      {
        CHECK(clean == text);
      }
    }
  }

  GIVEN("a malformed annotation")
  {
    std::string text = R"(<write file="test.cpp">content without closing tag)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_write_annotations(text);
      THEN("it is ignored")
      {
        CHECK(actions.empty());
      }
    }
  }
}

SCENARIO("parsing str_replace annotations")
{
  GIVEN("a response with a str_replace annotation")
  {
    std::string text = R"(Fix:
<str_replace path="src/main.cpp">
<old>int x = 1;</old>
<new>int x = 2;</new>
</str_replace>
Done.)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_str_replace_annotations(text);
      THEN("one action is found")
      {
        CHECK(actions.size() == 1);
        CHECK(actions[0].path == "src/main.cpp");
        CHECK(actions[0].old_str == "int x = 1;");
        CHECK(actions[0].new_str == "int x = 2;");
      }
    }

    WHEN("annotations are stripped")
    {
      auto clean = strip_annotations(text);
      THEN("tag is replaced with summary")
      {
        CHECK(clean.find("<str_replace") == std::string::npos);
        CHECK(clean.find("[proposed: str_replace src/main.cpp]") != std::string::npos);
      }
    }
  }
}

SCENARIO("parsing read annotations")
{
  GIVEN("a read annotation with line range")
  {
    std::string text = R"(Let me check: <read path="src/main.cpp" lines="10-20"/>)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_read_annotations(text);
      THEN("one action is found")
      {
        CHECK(actions.size() == 1);
        CHECK(actions[0].path == "src/main.cpp");
        CHECK(actions[0].from_line == 10);
        CHECK(actions[0].to_line == 20);
        CHECK(actions[0].search.empty());
      }
    }

    WHEN("annotations are stripped")
    {
      auto clean = strip_annotations(text);
      THEN("tag is replaced with summary")
      {
        CHECK(clean.find("<read") == std::string::npos);
        CHECK(clean.find("[read src/main.cpp]") != std::string::npos);
      }
    }
  }

  GIVEN("a read annotation with search term")
  {
    std::string text = R"(<read path="src/repl.cpp" search="confirm_write"/>)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_read_annotations(text);
      THEN("search term is extracted")
      {
        CHECK(actions.size() == 1);
        CHECK(actions[0].search == "confirm_write");
        CHECK(actions[0].from_line == 0);
      }
    }
  }

  GIVEN("a read annotation with malformed line range")
  {
    std::string text = R"(<read path="src/main.cpp" lines="x-y"/>)";

    WHEN("annotations are parsed")
    {
      auto actions = parse_read_annotations(text);
      THEN("it does not crash and defaults to 0")
      {
        CHECK(actions.size() == 1);
        CHECK(actions[0].from_line == 0);
        CHECK(actions[0].to_line == 0);
      }
    }
  }
}
