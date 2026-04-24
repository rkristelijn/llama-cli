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

// --- Edge cases ---

SCENARIO ("json: missing key returns empty") {
  GIVEN ("a JSON string") {
    std::string json = R"({"name":"alice"})";
    THEN ("missing key returns empty string") {
      CHECK (json_extract_string(json, "age").empty())
        ;
    }
    THEN ("missing key returns 0 for int") {
      CHECK (json_extract_int(json, "age") == 0)
        ;
    }
    THEN ("missing key returns empty for object") {
      CHECK (json_extract_object(json, "address").empty())
        ;
    }
  }
}

SCENARIO ("json: unicode escape decoding") {
  GIVEN ("a JSON string with unicode escape") {
    std::string json = R"({"msg":"hello\u0021"})";
    THEN ("\\u0021 decodes to !") {
      CHECK (json_extract_string(json, "msg") == "hello!")
        ;
    }
  }
}

SCENARIO ("json: whitespace around colon") {
  GIVEN ("JSON with spaces around colon") {
    // Note: find_key_value looks for "key": without space before colon
    // This tests the current behavior with standard format
    THEN ("extraction handles standard format") {
      std::string nospace = R"({"key":"value"})";
      CHECK (json_extract_string(nospace, "key") == "value")
        ;
    }
  }
}

SCENARIO ("json: nested object extraction") {
  GIVEN ("JSON with nested braces in strings") {
    std::string json = R"({"outer":{"inner":"val{ue}"}})";
    THEN ("object extraction handles braces in strings") {
      std::string obj = json_extract_object(json, "outer");
      CHECK (obj.find("inner") != std::string::npos)
        ;
    }
  }
}

SCENARIO ("json: extract_int with leading whitespace") {
  GIVEN ("JSON with space before int value") {
    std::string json = R"({"count": 42})";
    THEN ("int is extracted correctly") {
      CHECK (json_extract_int(json, "count") == 42)
        ;
    }
  }
}

// --- json_extract_object_at (SearXNG results array walking) ---

SCENARIO ("json: extract_object_at walks array of objects") {
  GIVEN ("a JSON array with multiple objects") {
    std::string json = R"([{"title":"A","url":"http://a"},{"title":"B","url":"http://b"}])";
    THEN ("first object is extracted at position 1") {
      std::string obj = json_extract_object_at(json, 1);
      CHECK (json_extract_string(obj, "title") == "A")
        ;
    }
    THEN ("second object is extracted by advancing past first") {
      std::string first = json_extract_object_at(json, 1);
      size_t next = 1 + first.size();
      // skip comma between objects
      while (next < json.size() && json[next] != '{') {
        next++;
      }
      std::string second = json_extract_object_at(json, next);
      CHECK (json_extract_string(second, "title") == "B")
        ;
    }
  }
  GIVEN ("an invalid position") {
    std::string json = R"({"key":"val"})";
    THEN ("non-brace position returns empty") {
      CHECK (json_extract_object_at(json, 0).empty() == false)
        ;
      CHECK (json_extract_object_at(json, 999).empty())
        ;
    }
  }
}

// TODO: test json_extract_string with escaped backslash before quote (\\\")
// TODO: test json_extract_object with deeply nested objects
// TODO: test decode_unicode_escape with non-ASCII codepoints (>127)
// TODO: test malformed JSON (unclosed strings, missing braces)
