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
