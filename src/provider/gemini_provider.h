/**
 * @file gemini_provider.h
 * @brief Gemini CLI provider — Google's AI via subprocess (ADR-020).
 *
 * Invokes `gemini -p "<prompt>"` in headless mode for each request.
 * Collapses conversation history into a single prompt string.
 */

#ifndef GEMINI_PROVIDER_H
#define GEMINI_PROVIDER_H

#include "provider/provider.h"

/// Provider that delegates to the Gemini CLI tool (Google AI)
class GeminiProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;
};

#endif  // GEMINI_PROVIDER_H
