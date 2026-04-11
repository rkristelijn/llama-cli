/**
 * @file test_helpers.h
 * @brief Shared test helpers: MockLLM, test_cfg
 */
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <string>
#include <vector>

#include "config/config.h"
#include "repl/repl.h"

/** Smart mock: echoes last user message, tracks calls and history */
struct MockLLM {
  int calls = 0;
  std::vector<Message> last_history;
  std::string fixed_response;

  std::string operator()(const std::vector<Message>& msgs) {
    calls++;
    last_history = msgs;
    if (!fixed_response.empty()) {
      return fixed_response;
    }
    return "echo: " + msgs.back().content;
  }
};

/** Config with empty system prompt for cleaner test assertions */
inline Config test_cfg() {
  Config c;
  c.system_prompt = "";
  return c;
}

#endif
