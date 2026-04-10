// repl.h — Interactive REPL loop
// Reads user input, sends to Ollama with conversation history.
// Uses injectable chat function for testability (ADR-008).

#ifndef REPL_H
#define REPL_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "ollama.h"

// Injectable function type for chat with history
using ChatFn = std::function<std::string(const std::vector<Message>&)>;

// Run the REPL loop with conversation memory.
// Returns number of prompts processed.
int run_repl(ChatFn chat, std::istream& in = std::cin, std::ostream& out = std::cout);

#endif
