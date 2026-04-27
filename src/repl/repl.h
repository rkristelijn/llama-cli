// Interactive REPL loop for Ollama with conversation history and testability features.
// Reads user input, sends to Ollama with conversation history.
// Uses injectable chat function for testability (ADR-008).

#ifndef REPL_H
#define REPL_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "config/config.h"
#include "exec/hardware.h"
#include "ollama/ollama.h"

// Injectable function types for testability (ADR-008).
// Production code passes the real implementation; tests pass mocks.
// This avoids network calls in unit tests while keeping the code simple.
using ChatFn = std::function<std::string(const std::vector<Message>&)>;
using StreamChatFn = std::function<std::string(const std::vector<Message>&, StreamCallback on_token)>;
using ModelsFn = std::function<std::vector<std::string>(const Config&)>;
using HardwareFn = std::function<HardwareInfo()>;

// Run the REPL loop with conversation memory.
// system_prompt is added as first message if non-empty.
// models_fn defaults to get_available_models (real HTTP call to Ollama).
// Returns number of prompts processed.
int run_repl(ChatFn chat, const Config& cfg = Config{}, std::istream& in = std::cin, std::ostream& out = std::cout,
             ModelsFn models_fn = get_available_models, StreamChatFn stream_chat = nullptr, HardwareFn hw_fn = detect_hardware);

#endif
