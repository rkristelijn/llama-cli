/**
 * @file ollama_provider.h
 * @brief Ollama backend implementation of LLMProvider (ADR-020).
 *
 * Wraps the existing ollama_* free functions behind the provider interface.
 * Each instance targets a single host:port — multi-host uses multiple instances.
 */

#ifndef OLLAMA_PROVIDER_H
#define OLLAMA_PROVIDER_H

#include "config/config.h"
#include "provider/provider.h"

/// Ollama provider — delegates to ollama_chat, ollama_chat_stream, etc.
class OllamaProvider : public LLMProvider {
 public:
  /// Construct with a config snapshot (captures host, port, model, timeout).
  explicit OllamaProvider(const Config& cfg);

  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;

 private:
  Config cfg_;  ///< Snapshot of config at construction time
};

#endif  // OLLAMA_PROVIDER_H
