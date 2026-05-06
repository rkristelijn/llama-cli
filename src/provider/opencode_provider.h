/**
 * @file opencode_provider.h
 * @brief OpenCode CLI provider — subprocess wrapper (ADR-096).
 *
 * Invokes the `opencode` binary for chat. Registered when `which opencode` succeeds.
 */

#ifndef OPENCODE_PROVIDER_H
#define OPENCODE_PROVIDER_H

#include "provider/provider.h"

/// Provider that delegates to the OpenCode CLI tool.
class OpenCodeProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;
};

#endif  // OPENCODE_PROVIDER_H
