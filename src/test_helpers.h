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
  int calls = 0;                      ///< Number of times chat was called
  std::vector<Message> last_history;  ///< History of the last call
  std::string fixed_response;         ///< Response to return if not empty

  /** Mock chat function: echoes the last user message */
  std::string operator()(const std::vector<Message>& msgs, Trace* = nullptr) {
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
