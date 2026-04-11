// ollama.h — Ollama API client
// Handles HTTP communication with a local Ollama instance.

#ifndef OLLAMA_H
#define OLLAMA_H

#include <string>
#include <vector>

#include "config/config.h"

/// A single message in a conversation
struct Message {
  std::string role;     ///< Message role: "system", "user", or "assistant"
  std::string content;  ///< Message text content
};

// Send a one-shot prompt (no history). Uses /api/generate.
std::string ollama_generate(const Config& cfg, const std::string& prompt);

// Send a conversation to Ollama. Uses /api/chat.
// Returns the assistant's response text.
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages);

#endif
