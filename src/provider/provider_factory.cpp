/**
 * @file provider_factory.cpp
 * @brief Factory implementation — maps config.provider to concrete LLMProvider.
 */

#include "provider/provider_factory.h"

#include <cstdlib>
#include <stdexcept>

#include "provider/gemini_provider.h"
#include "provider/multi_host_provider.h"
#include "provider/ollama_provider.h"
#include "provider/tgpt_provider.h"

/// MockProvider for testing — echoes prompts back or uses env var response.
/// Keeps tests fast and network-free (ADR-008).
class MockProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override { return mock_response(messages); }

  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override {
    std::string response = mock_response(messages);
    if (on_token) {
      on_token(response);
    }
    return response;
  }

  std::vector<std::string> list_models() override { return {"mock-model"}; }
  std::vector<ModelInfo> get_model_info() override { return {{"mock-model", "1B", "Q4", "mock", 0.1}}; }
  bool is_model_running(const std::string& /*model_name*/) override { return true; }
  std::string name() const override { return "mock"; }
  std::string host() const override { return "localhost:0"; }

 private:
  /// Return env-configured response or echo the last message.
  static std::string mock_response(const std::vector<Message>& messages) {
    const char* env = std::getenv("LLAMA_CLI_MOCK_RESPONSE");
    if (env) {
      return env;
    }
    return "mock response: " + (messages.empty() ? "" : messages.back().content);
  }
};

std::unique_ptr<LLMProvider> create_provider(const Config& cfg) {
  if (cfg.provider == "mock") {
    return std::make_unique<MockProvider>();
  }
  if (cfg.provider == "tgpt") {
    return std::make_unique<TgptProvider>();
  }
  if (cfg.provider == "gemini") {
    return std::make_unique<GeminiProvider>();
  }
  if (cfg.provider == "ollama" || cfg.provider.empty()) {
    // Multi-host: if OLLAMA_HOSTS has multiple entries, use routing provider
    if (cfg.hosts.size() > 1) {
      return std::make_unique<MultiHostProvider>(cfg.hosts, cfg.model);
    }
    return std::make_unique<OllamaProvider>(cfg);
  }
  throw std::runtime_error("Unknown provider: " + cfg.provider);
}
