// repl.h — Interactive REPL loop
// Reads user input, sends to Ollama with conversation history.
// Uses injectable chat function for testability (ADR-008).

#ifndef REPL_H
#define REPL_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "config/config.h"
#include "ollama/ollama.h"
#include "trace/trace.h"

// Injectable function type for chat with history
using ChatFn = std::function<std::string(const std::vector<Message>&, Trace*)>;

// Run the REPL loop with conversation memory.
// system_prompt is added as first message if non-empty.
// Returns number of prompts processed.
int run_repl(ChatFn chat, const Config& cfg = Config{}, std::istream& in = std::cin, std::ostream& out = std::cout);

#endif
