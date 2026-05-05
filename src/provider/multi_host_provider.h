/**
 * @file multi_host_provider.h
 * @brief Routes requests across multiple Ollama hosts with model-match and failover.
 *
 * Strategy: find the host that has the requested model, failover to next on error.
 * Health checks are on-demand (probe before request) — keeps the CLI single-threaded.
 */

#ifndef MULTI_HOST_PROVIDER_H
#define MULTI_HOST_PROVIDER_H

#include <memory>
#include <string>
#include <vector>

#include "provider/ollama_provider.h"
#include "provider/provider.h"

/// Routes LLM requests across multiple Ollama hosts.
/// Model-match strategy: sends to the host that has the model.
/// Auto-failover: tries next host on connection failure.
class MultiHostProvider : public LLMProvider {
 public:
  /// Construct from a list of "host:port" strings.
  /// Probes each host for available models on construction.
  explicit MultiHostProvider(const std::vector<std::string>& hosts, const std::string& model);

  std::string chat(const std::vector<Message>& messages) override;
  std::string chat_stream(const std::vector<Message>& messages, StreamCallback on_token) override;
  std::vector<std::string> list_models() override;
  std::vector<ModelInfo> get_model_info() override;
  bool is_model_running(const std::string& model_name) override;
  std::string name() const override;
  std::string host() const override;

 private:
  struct HostEntry {
    std::unique_ptr<OllamaProvider> provider;
    std::vector<std::string> models;  ///< Cached model list
    bool healthy = true;              ///< Last known health status
  };

  std::vector<HostEntry> hosts_;
  std::string model_;   ///< Target model name
  int active_idx_ = 0;  ///< Index of currently active host

  /// Find host that has the target model, or first healthy host.
  int find_host_for_model(const std::string& model_name);

  /// Try next healthy host (failover).
  bool failover();
};

#endif  // MULTI_HOST_PROVIDER_H
