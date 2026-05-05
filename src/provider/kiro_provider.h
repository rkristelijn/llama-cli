/**
 * @file kiro_provider.h
 * @brief Kiro CLI provider — cloud LLM via kiro-cli-chat subprocess.
 *
 * Uses --no-interactive --wrap=never for headless mode.
 * Supports --list-models for model discovery with cost info.
 */

#ifndef KIRO_PROVIDER_H
#define KIRO_PROVIDER_H

#include "provider/provider.h"

/// Provider that delegates to kiro-cli-chat (Claude, DeepSeek, etc. via credits)
class KiroProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;
};

#endif  // KIRO_PROVIDER_H
