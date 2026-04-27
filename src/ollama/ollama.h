// API client for local instance handling HTTP communication and conversation management with Ollama.
// Handles HTTP communication with a local Ollama instance.

#ifndef OLLAMA_H
#define OLLAMA_H

#include <functional>
#include <string>
#include <vector>

#include "config/config.h"

/// A single message in a conversation
struct Message {
  std::string role;         ///< Message role: "system", "user", or "assistant"
  std::string content;      ///< Message text content
  std::string rating = "";  ///< Rating: "positive", "negative", "saved", or empty (default)
};

// Send a one-shot prompt (no history). Uses /api/generate.
std::string ollama_generate(const Config& cfg, const std::string& prompt);

// Send a conversation to Ollama. Uses /api/chat.
// Returns the assistant's response text.
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages);

/// Callback invoked for each token during streaming
using StreamCallback = std::function<bool(const std::string& token)>;

// Stream a conversation response from Ollama. Uses /api/chat with stream:true.
// Calls on_token for each token. Returns the full response text.
// on_token should return true to continue, false to abort.
std::string ollama_chat_stream(const Config& cfg, const std::vector<Message>& messages, StreamCallback on_token);

// Fetch list of available models from Ollama server.
// Returns vector of model names. Empty on error.
std::vector<std::string> get_available_models(const Config& cfg);

// Check if a model is currently loaded in Ollama's memory.
bool is_model_running(const Config& cfg, const std::string& model_name);

/// Model metadata from Ollama /api/tags
struct ModelInfo {
  std::string name;    ///< Model name (e.g. "gemma4:26b")
  std::string params;  ///< Parameter size (e.g. "26.0B")
  std::string quant;   ///< Quantization (e.g. "Q4_K_M")
  std::string family;  ///< Model family (e.g. "gemma4")
  double size_gb = 0;  ///< Size on disk in GB
};

/// Fetch model list with metadata from Ollama server
std::vector<ModelInfo> get_model_info(const Config& cfg);

#endif
