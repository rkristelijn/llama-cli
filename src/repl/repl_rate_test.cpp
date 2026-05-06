/**
 * @file repl_rate_test.cpp
 * @brief Unit tests for /rate command.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "config/config.h"
#include "repl/repl.h"
#include "test_helpers.h"

// Mock chat: returns last user message prefixed with "echo: "
static std::string echo_chat(const std::vector<Message>& messages) { return "echo: " + messages.back().content; }

SCENARIO ("REPL /rate command") {
  GIVEN ("user rates last response via /rate") {
    std::istringstream in("hello\n/rate last +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates last response as negative") {
    std::istringstream in("hello\n/rate last -\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: negative]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user saves last response for review") {
    std::istringstream in("hello\n/rate last s\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("rating is confirmed") {
      CHECK (out.str().find("[rated: saved]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user lists rated responses after /rate") {
    std::istringstream in("hello\n/rate last +\n/rate list\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("list shows rated responses") {
      CHECK (out.str().find("1. [positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user lists with no ratings") {
    std::istringstream in("hello\n/rate list\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("no rated responses message is shown") {
      CHECK (out.str().find("[no rated responses]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user provides invalid /rate syntax") {
    std::istringstream in("/rate\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("usage help is shown") {
      CHECK (out.str().find("Usage: /rate") != std::string::npos)
        ;
    }
  }

  GIVEN ("user provides invalid rating value") {
    std::istringstream in("hello\n/rate last x\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("invalid rating message is shown") {
      CHECK (out.str().find("Invalid rating") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates a specific response by index") {
    std::istringstream in("first\nsecond\n/rate 1 +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("first response is rated") {
      CHECK (out.str().find("[rated: positive]") != std::string::npos)
        ;
    }
  }

  GIVEN ("user rates with no prior response") {
    std::istringstream in("/rate last +\nexit\n");
    std::ostringstream out;
    run_repl(echo_chat, test_cfg(), in, out);
    THEN ("response not found message is shown") {
      CHECK (out.str().find("Response not found") != std::string::npos)
        ;
    }
  }
}
