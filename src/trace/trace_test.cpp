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
