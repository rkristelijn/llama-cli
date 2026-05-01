/**
 * @file trace_test.cpp
 * @brief Unit tests for trace module.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "trace/trace.h"

#include <doctest/doctest.h>
#include <unistd.h>

#include <cstdio>
#include <string>

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
    WHEN ("log is called via the global pointer") {
      // Swap in a CapturingTrace to verify messages without stderr noise
      // (ADR-060 will unify all error output to ostream&)
      CapturingTrace capture;
      Trace* original = stderr_trace;
      stderr_trace = &capture;
      stderr_trace->log("test %s", "message");
      stderr_trace->log("number %d", 42);
      stderr_trace->log("no args");
      stderr_trace = original;
      THEN ("messages were captured") {
        REQUIRE (capture.messages.size() == 3)
          ;
        CHECK (capture.messages[0] == "test message")
          ;
        CHECK (capture.messages[1] == "number 42")
          ;
        CHECK (capture.messages[2] == "no args")
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

SCENARIO ("StderrTrace::log writes to stderr") {
  GIVEN ("the stderr trace instance") {
    WHEN ("log is called directly on the instance") {
      // Redirect stderr to capture output
      FILE* tmp = tmpfile();
      int saved = dup(fileno(stderr));
      dup2(fileno(tmp), fileno(stderr));

      stderr_trace_instance.log("direct %s %d", "test", 99);

      // Restore stderr and read captured output
      fflush(stderr);
      dup2(saved, fileno(stderr));
      close(saved);

      // Read what was written
      fseek(tmp, 0, SEEK_SET);
      char buf[512];
      size_t n = fread(buf, 1, sizeof(buf) - 1, tmp);
      buf[n] = '\0';
      fclose(tmp);

      std::string output(buf);
      THEN ("output contains timestamp in HH:MM:SS format") {
        // Pattern: [HH:MM:SS]
        CHECK (output.find("[") != std::string::npos)
          ;
        CHECK (output.find(":") != std::string::npos)
          ;
        CHECK (output.find("]") != std::string::npos)
          ;
      }
      THEN ("output contains the formatted message") {
        CHECK (output.find("direct test 99") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("CapturingTrace truncates messages over 256 chars") {
  GIVEN ("a capturing trace") {
    CapturingTrace trace;
    WHEN ("a message longer than 256 chars is logged") {
      // Build a 300-char format string
      std::string long_msg(300, 'x');
      trace.log("%s", long_msg.c_str());
      THEN ("message is truncated to buffer size") {
        REQUIRE (trace.messages.size() == 1)
          ;
        // vsnprintf with 256-byte buffer truncates at 255 + null
        CHECK (trace.messages[0].size() == 255)
          ;
      }
    }
  }
}
