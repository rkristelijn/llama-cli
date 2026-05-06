/**
 * @file provider.h
 * @brief Abstract LLM provider interface (ADR-020).
 *
 * Decouples the REPL and sync mode from any specific backend (Ollama, cloud APIs, tgpt).
 * Implementations handle HTTP/subprocess details; callers only see this interface.
 */

#ifndef PROVIDER_H
#define PROVIDER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ollama/ollama.h"  // Message, ModelInfo, StreamCallback types

/// Abstract base class for LLM providers (ADR-020).
/// Each provider wraps a specific backend (Ollama, OpenAI-compatible, tgpt, etc.).
class LLMProvider {
 public:
  virtual ~LLMProvider() = default;

  /// Send a conversation and return the full response text.
  virtual std::string chat(const std::vector<Message>& messages) = 0;

  /// Stream a conversation response, calling on_token for each chunk.
  /// Returns the full concatenated response.
  virtual std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) = 0;

  /// List available model names on this provider.
  virtual std::vector<std::string> list_models() = 0;

  /// Fetch detailed model metadata.
  virtual std::vector<ModelInfo> get_model_info() = 0;

  /// Check if a specific model is loaded/ready.
  virtual bool is_model_running(const std::string& model_name) = 0;

  /// Provider display name (e.g., "ollama", "openai", "tgpt").
  virtual std::string name() const = 0;

  /// Host identifier for logging (e.g., "<hostname>:11434").
  virtual std::string host() const = 0;
};

#endif  // PROVIDER_H
