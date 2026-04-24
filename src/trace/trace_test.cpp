/**
 * @file trace_test.cpp
 * @brief Unit tests for trace module.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "trace/trace.h"

#include <doctest/doctest.h>

SCENARIO ("CapturingTrace captures messages") {
  GIVEN ("a capturing trace instance") {
    CapturingTrace trace;
    WHEN ("a message is logged") {
      trace.log("hello %s %d", "world", 42);
      THEN ("it is captured") {
        REQUIRE (trace.messages.size() == 1)
          ;
        CHECK (trace.messages[0] == "hello world 42")
          ;
      }
    }
    WHEN ("multiple messages are logged") {
      trace.log("first");
      trace.log("second %d", 2);
      THEN ("all are captured") {
        CHECK (trace.messages.size() == 2)
          ;
        CHECK (trace.messages[0] == "first")
          ;
        CHECK (trace.messages[1] == "second 2")
          ;
      }
    }
  }
}

SCENARIO ("StderrTrace exists") {
  GIVEN ("the global stderr trace") {
    THEN ("it is not null") {
      CHECK (stderr_trace != nullptr)
        ;
    }
  }
}

// --- StderrTrace output test ---

SCENARIO ("StderrTrace writes to stderr") {
  GIVEN ("the stderr trace instance") {
    WHEN ("log is called") {
      // StderrTrace writes to stderr — we can't easily capture it,
      // but we verify it doesn't crash with various format strings
      stderr_trace_instance.log("test %s", "message");
      stderr_trace_instance.log("number %d", 42);
      stderr_trace_instance.log("no args");
      THEN ("no crash occurred") {
        CHECK (true)
          ;
      }
    }
  }
}

SCENARIO ("CapturingTrace handles empty format") {
  GIVEN ("a capturing trace") {
    CapturingTrace trace;
    WHEN ("empty string is logged") {
      trace.log("");
      THEN ("empty message is captured") {
        REQUIRE (trace.messages.size() == 1)
          ;
        CHECK (trace.messages[0].empty())
          ;
      }
    }
  }
}

// TODO: test StderrTrace output format (timestamp + message) — needs stderr capture
// TODO: test CapturingTrace with buffer overflow (>256 char message)
