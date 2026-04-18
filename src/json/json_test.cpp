// test_json.cpp — Unit tests for JSON extraction
// Uses Given/When/Then style per ADR-008

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "json/json.h"

#include <doctest/doctest.h>

SCENARIO ("json string extraction") {
  GIVEN ("a simple JSON object") {
    std::string json = "{\"response\":\"hello world\"}";
    WHEN ("a known key is extracted") {
      THEN ("the value is returned") {
        CHECK (json_extract_string(json, "response") == "hello world")
          ;
      }
    }
    WHEN ("a missing key is extracted") {
      THEN ("empty string is returned") {
        CHECK (json_extract_string(json, "missing") == "")
          ;
      }
    }
  }

  GIVEN ("a JSON string with escaped characters") {
    WHEN ("it contains a newline") {
      std::string json = "{\"response\":\"line1\\nline2\"}";
      THEN ("the newline is unescaped") {
        CHECK (json_extract_string(json, "response") == "line1\nline2")
          ;
      }
    }
    WHEN ("it contains escaped quotes") {
      std::string json = "{\"response\":\"say \\\"hello\\\"\"}";
      THEN ("the quotes are unescaped") {
        CHECK (json_extract_string(json, "response") == "say \"hello\"")
          ;
      }
    }
    WHEN ("it contains a tab") {
      std::string json = "{\"response\":\"col1\\tcol2\"}";
      THEN ("the tab is unescaped") {
        CHECK (json_extract_string(json, "response") == "col1\tcol2")
          ;
      }
    }
    WHEN ("it contains a carriage return") {
      std::string json = "{\"response\":\"line1\\rline2\"}";
      THEN ("the carriage return is unescaped") {
        CHECK (json_extract_string(json, "response") == "line1\rline2")
          ;
      }
    }
    WHEN ("it contains a backspace") {
      std::string json = "{\"response\":\"ab\\bc\"}";
      THEN ("the backspace is unescaped") {
        CHECK (json_extract_string(json, "response") == "ab\bc")
          ;
      }
    }
    WHEN ("it contains a form feed") {
      std::string json = "{\"response\":\"page1\\fpage2\"}";
      THEN ("the form feed is unescaped") {
        CHECK (json_extract_string(json, "response") == "page1\fpage2")
          ;
      }
    }
    WHEN ("it contains an escaped backslash") {
      std::string json = "{\"response\":\"a\\\\b\"}";
      THEN ("the backslash is unescaped") {
        CHECK (json_extract_string(json, "response") == "a\\b")
          ;
      }
    }
    WHEN ("it contains an escaped forward slash") {
      std::string json = "{\"response\":\"a\\/b\"}";
      THEN ("the forward slash is unescaped") {
        CHECK (json_extract_string(json, "response") == "a/b")
          ;
      }
    }
  }

  GIVEN ("a JSON string with unicode escapes") {
    WHEN ("it contains \\u003c and \\u003e") {
      std::string json = "{\"response\":\"\\u003cwrite\\u003ehello\\u003c/write\\u003e\"}";
      THEN ("unicode is decoded to < and >") {
        CHECK (json_extract_string(json, "response") == "<write>hello</write>")
          ;
      }
    }
  }

  GIVEN ("a larger JSON object with multiple keys") {
    std::string json = "{\"model\":\"gemma4\",\"response\":\"the answer\",\"done\":true}";
    WHEN ("response is extracted") {
      THEN ("only the response value is returned") {
        CHECK (json_extract_string(json, "response") == "the answer")
          ;
      }
    }
  }
}

SCENARIO ("json_extract_object") {
  GIVEN ("a nested JSON object") {
    std::string json = R"({"message":{"role":"assistant","content":"hello"},"done":true})";
    WHEN ("message is extracted") {
      auto obj = json_extract_object(json, "message");
      THEN ("the full object is returned") {
        CHECK (obj.find("role") != std::string::npos)
          ;
        CHECK (obj.find("assistant") != std::string::npos)
          ;
        CHECK (obj.find("hello") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("braces inside a string value") {
    std::string json = R"({"message":{"content":"use {x} here"}})";
    WHEN ("message is extracted") {
      auto obj = json_extract_object(json, "message");
      THEN ("braces in strings are ignored") {
        CHECK (obj.find("use {x} here") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("escaped quotes inside a string value") {
    std::string json = R"({"message":{"content":"say \"hello\""}})";
    WHEN ("message is extracted") {
      auto obj = json_extract_object(json, "message");
      THEN ("escaped quotes are handled") {
        CHECK (obj.find("hello") != std::string::npos)
          ;
      }
    }
  }

  GIVEN ("a missing key") {
    std::string json = R"({"other":{"a":1}})";
    WHEN ("message is extracted") {
      THEN ("empty string is returned") {
        CHECK (json_extract_object(json, "message").empty())
          ;
      }
    }
  }
}
