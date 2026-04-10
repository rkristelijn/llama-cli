// test_json.cpp — Unit tests for JSON extraction
// Uses Given/When/Then style per ADR-008

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "json.h"

SCENARIO("json string extraction") {
  GIVEN("a simple JSON object") {
    std::string json = R"({"response":"hello world"})";
    WHEN("a known key is extracted") {
      THEN("the value is returned") { CHECK(json_extract_string(json, "response") == "hello world"); }
    }
    WHEN("a missing key is extracted") {
      THEN("empty string is returned") { CHECK(json_extract_string(json, "missing") == ""); }
    }
  }

  GIVEN("a JSON string with escaped characters") {
    WHEN("it contains a newline") {
      std::string json = R"({"response":"line1\nline2"})";
      THEN("the newline is unescaped") { CHECK(json_extract_string(json, "response") == "line1\nline2"); }
    }
    WHEN("it contains escaped quotes") {
      std::string json = R"({"response":"say \"hello\""})";
      THEN("the quotes are unescaped") { CHECK(json_extract_string(json, "response") == "say \"hello\""); }
    }
  }

  GIVEN("a JSON string with unicode escapes") {
    WHEN("it contains \\u003c and \\u003e") {
      std::string json = R"({"response":"\u003cwrite\u003ehello\u003c/write\u003e"})";
      THEN("unicode is decoded to < and >") { CHECK(json_extract_string(json, "response") == "<write>hello</write>"); }
    }
  }

  GIVEN("a larger JSON object with multiple keys") {
    std::string json = R"({"model":"gemma4","response":"the answer","done":true})";
    WHEN("response is extracted") {
      THEN("only the response value is returned") { CHECK(json_extract_string(json, "response") == "the answer"); }
    }
  }
}
