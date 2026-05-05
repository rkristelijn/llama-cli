/**
 * @file tgpt_provider.h
 * @brief tgpt CLI provider — free cloud LLM via subprocess (ADR-020).
 *
 * Invokes `tgpt -q -p "<prompt>"` for each request.
 * Collapses conversation history into a single prompt string.
 */

#ifndef TGPT_PROVIDER_H
#define TGPT_PROVIDER_H

#include "provider/provider.h"

/// Provider that delegates to the tgpt CLI tool (Pollinations.AI, FreeGpt, etc.)
class TgptProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;
};

#endif  // TGPT_PROVIDER_H
