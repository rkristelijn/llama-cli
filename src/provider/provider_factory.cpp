/**
 * @file provider_factory.cpp
 * @brief Factory implementation — maps config.provider to concrete LLMProvider.
 */

#include "provider/provider_factory.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "provider/gemini_provider.h"
#include "provider/kiro_provider.h"
#include "provider/multi_host_provider.h"
#include "provider/ollama_provider.h"
#include "provider/opencode_provider.h"
#include "provider/tgpt_provider.h"

/// MockProvider for testing — echoes prompts back or uses env var response.
/// Keeps tests fast and network-free (ADR-008).
class MockProvider : public LLMProvider {
 public:
  std::string chat(const std::vector<Message>& messages) override { return mock_response(messages); }

  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override {
    std::string response = mock_response(messages);
    // Optional delay to simulate thinking time (shows spinner in demos)
    const char* delay_env = std::getenv("LLAMA_CLI_MOCK_DELAY_MS");
    if (delay_env) {
      std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(delay_env)));
    }
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
  /// Return scripted response (from file), env-configured response, or echo.
  /// LLAMA_CLI_MOCK_SCRIPT: file with labeled responses. Format:
  ///   [keyword]
  ///   response text here
  ///   ---
  ///   [other keyword]
  ///   other response
  ///   ---
  /// Matches the first block whose keyword appears in the user prompt.
  /// LLAMA_CLI_MOCK_RESPONSE: static response for all prompts.
  /// Fallback: echo the last user message.
  static std::string mock_response(const std::vector<Message>& messages) {
    std::string prompt = messages.empty() ? "" : messages.back().content;
    const char* script = std::getenv("LLAMA_CLI_MOCK_SCRIPT");
    if (script) {
      return match_scripted_response(script, prompt);
    }
    const char* env = std::getenv("LLAMA_CLI_MOCK_RESPONSE");
    if (env) {
      return env;
    }
    return "mock response: " + prompt;
  }

  /// Match response by keyword in prompt. Falls back to [default] block or echo.
  static std::string match_scripted_response(const char* path, const std::string& prompt) {
    static std::vector<std::pair<std::string, std::string>> entries;
    if (entries.empty()) {
      std::ifstream file(path);
      std::string line;
      std::string key;
      std::string body;
      while (std::getline(file, line)) {
        if (line.size() > 2 && line.front() == '[' && line.back() == ']') {
          if (!key.empty()) entries.emplace_back(key, body);
          key = line.substr(1, line.size() - 2);
          body.clear();
        } else if (line == "---") {
          if (!key.empty()) entries.emplace_back(key, body);
          key.clear();
          body.clear();
        } else {
          if (!body.empty()) body += "\n";
          body += line;
        }
      }
      if (!key.empty()) entries.emplace_back(key, body);
    }
    // Find first entry whose key appears in the prompt (case-insensitive)
    std::string lower_prompt = prompt;
    for (auto& ch : lower_prompt) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    std::string fallback;
    for (const auto& [key, response] : entries) {
      if (key == "default") {
        fallback = response;
        continue;
      }
      std::string lower_key = key;
      for (auto& ch : lower_key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
      if (lower_prompt.find(lower_key) != std::string::npos) {
        return response;
      }
    }
    return fallback.empty() ? ("mock response: " + prompt) : fallback;
  }
};

/// Create the appropriate LLMProvider based on config.provider setting.
/// Supports: mock (testing), tgpt, gemini, kiro, ollama (default).
/// Multi-host Ollama uses MultiHostProvider for load balancing (ADR-078).
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
  if (cfg.provider == "kiro-cli" || cfg.provider == "kiro" || cfg.provider == "amazon-q") {
    // All three names map to KiroProvider which uses kiro-cli-chat subprocess.
    // "amazon-q" is the user-facing name, "kiro-cli"/"kiro" are aliases.
    // The provider handles model selection via --model flag internally.
    return std::make_unique<KiroProvider>();
  }
  if (cfg.provider == "opencode" || cfg.provider == "oc") {
    return std::make_unique<OpenCodeProvider>();
  }
  if (cfg.provider == "ollama" || cfg.provider.empty()) {
    // Multi-host: if OLLAMA_HOSTS has multiple entries, use routing provider
    if (cfg.hosts.size() > 1) {
      if (cfg.trace) {
        std::cerr << "[TRACE] Factory: MultiHostProvider hosts=" << cfg.hosts.size() << " model=" << cfg.model << "\n";
      }
      return std::make_unique<MultiHostProvider>(cfg.hosts, cfg.model);
    }
    if (cfg.trace) {
      std::cerr << "[TRACE] Factory: OllamaProvider host=" << cfg.host << ":" << cfg.port << " model=" << cfg.model << "\n";
    }
    return std::make_unique<OllamaProvider>(cfg);
  }
  throw std::runtime_error("Unknown provider: " + cfg.provider);
}
