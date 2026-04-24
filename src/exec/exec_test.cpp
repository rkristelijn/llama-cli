// test_exec.cpp — Unit tests for command execution

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "exec/exec.h"

#include <doctest/doctest.h>

SCENARIO ("command execution") {
  GIVEN ("a simple command") {
    auto r = cmd_exec("echo hello", 5, 10000);
    THEN ("output is captured") {
      CHECK (r.output == "hello\n")
        ;
    }
    THEN ("exit code is 0") {
      CHECK (r.exit_code == 0)
        ;
    }
    THEN ("no timeout") {
      CHECK_FALSE (r.timed_out)
        ;
    }
  }

  GIVEN ("a failing command") {
    auto r = cmd_exec("false", 5, 10000);
    THEN ("exit code is non-zero") {
      CHECK (r.exit_code != 0)
        ;
    }
  }

  GIVEN ("a command with stderr") {
    auto r = cmd_exec("ls /nonexistent_path_xyz", 5, 10000);
    THEN ("stderr is captured") {
      CHECK (r.output.find("No such file") != std::string::npos)
        ;
    }
  }

  GIVEN ("output exceeding max_chars") {
    auto r = cmd_exec("seq 1 1000", 5, 50);
    THEN ("output is truncated") {
      CHECK (r.output.find("[truncated") != std::string::npos)
        ;
    }
    THEN ("output is within limit") {
      CHECK (r.output.size() < 200)
        ;
    }
  }

  GIVEN ("a command that times out") {
    // Produces output every second, timeout after 1s
    auto r = cmd_exec("for i in 1 2 3 4 5; do echo $i; sleep 1; done", 1, 10000);
    THEN ("timeout is detected") {
      CHECK (r.timed_out)
        ;
    }
    THEN ("partial output is captured") {
      CHECK (r.output.find("1") != std::string::npos)
        ;
    }
  }
}

// --- Timeout and truncation ---

SCENARIO ("command execution: timeout") {
  GIVEN ("a command that runs longer than the timeout") {
    WHEN ("executed with 1 second timeout") {
      // Use a loop that produces output — timeout is checked between reads
      auto r = cmd_exec("for i in $(seq 1 100); do echo $i; sleep 0.1; done", 1, 10000);
      THEN ("it is killed by timeout") {
        CHECK (r.timed_out)
          ;
        CHECK (r.output.find("timeout") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("command execution: output truncation") {
  GIVEN ("a command that produces lots of output") {
    WHEN ("executed with small max_chars") {
      auto r = cmd_exec("seq 1 10000", 5, 100);
      THEN ("output is truncated") {
        CHECK (r.output.find("truncated") != std::string::npos)
          ;
        CHECK (static_cast<int>(r.output.size()) < 200)
          ;
      }
    }
  }
}

SCENARIO ("command execution: failed command") {
  GIVEN ("a command that exits with non-zero") {
    WHEN ("executed") {
      auto r = cmd_exec("false", 5, 10000);
      THEN ("exit code is non-zero") {
        CHECK (r.exit_code != 0)
          ;
        CHECK (!r.timed_out)
          ;
      }
    }
  }
}

// TODO: test popen failure path (cmd_exec returns "Error: could not execute command")
// TODO: test WIFSIGNALED path (command killed by signal)
