/**
 * @file multi_host_provider.cpp
 * @brief Multi-host routing: model-match + failover across Ollama instances.
 *
 * On construction, probes each host for models (on-demand health check).
 * Routes to the host that has the requested model.
 * On connection failure, marks host unhealthy and tries the next one.
 */

#include "provider/multi_host_provider.h"

#include <algorithm>
#include <iostream>

#include "config/config.h"
#include "logging/logger.h"
#include "trace/trace.h"

MultiHostProvider::MultiHostProvider(const std::vector<std::string>& hosts, const std::string& model) : model_(model) {
  bool trace = Config::instance().trace;
  if (trace) {
    stderr_trace->log("[TRACE] MultiHost: creating with model=%s hosts=%zu\n", model.c_str(), hosts.size());
  }
  // Create a provider per host and probe for available models
  for (const auto& hp : hosts) {
    Config cfg;
    auto colon = hp.find(':');
    cfg.host = (colon != std::string::npos) ? hp.substr(0, colon) : hp;
    cfg.port = (colon != std::string::npos) ? hp.substr(colon + 1) : "11434";
    cfg.timeout = Config::instance().timeout;  // Use real timeout for chat requests
    cfg.model = model;                         // Pass model so chat requests include it
    cfg.trace = Config::instance().trace;
    cfg.no_color = Config::instance().no_color;

    auto prov = std::make_unique<OllamaProvider>(cfg);
    auto models = prov->list_models();
    bool healthy = !models.empty();

    if (trace) {
      stderr_trace->log("[TRACE] MultiHost: %s:%s healthy=%d models=%zu\n", cfg.host.c_str(), cfg.port.c_str(), healthy, models.size());
    }
    hosts_.push_back({std::move(prov), models, healthy});
  }

  // Select initial host based on model-match
  active_idx_ = find_host_for_model(model_);
  if (trace) {
    stderr_trace->log("[TRACE] MultiHost: active_idx=%d host=%s\n", active_idx_, hosts_[active_idx_].provider->host().c_str());
  }
}

int MultiHostProvider::find_host_for_model(const std::string& model_name) {
  // First: find a healthy host that has the model
  for (int i = 0; i < static_cast<int>(hosts_.size()); ++i) {
    if (!hosts_[i].healthy) {
      continue;
    }
    for (const auto& m : hosts_[i].models) {
      if (m == model_name) {
        return i;
      }
    }
  }
  // Fallback: first healthy host (model might need to be pulled)
  for (int i = 0; i < static_cast<int>(hosts_.size()); ++i) {
    if (hosts_[i].healthy) {
      return i;
    }
  }
  return 0;  // All unhealthy — try first anyway
}

bool MultiHostProvider::failover() {
  hosts_[active_idx_].healthy = false;
  // Find next healthy host
  for (int i = 1; i < static_cast<int>(hosts_.size()); ++i) {
    int idx = (active_idx_ + i) % static_cast<int>(hosts_.size());
    if (hosts_[idx].healthy) {
      active_idx_ = idx;
      return true;
    }
  }
  return false;  // No healthy hosts left
}

std::string MultiHostProvider::chat(const std::vector<Message>& messages) {
  std::string result = hosts_[active_idx_].provider->chat(messages);
  // Empty result with no content likely means connection failure
  if (result.empty() && failover()) {
    result = hosts_[active_idx_].provider->chat(messages);
  }
  return result;
}

std::string MultiHostProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] MultiHost: chat_stream → %s model=%s\n", hosts_[active_idx_].provider->host().c_str(), model_.c_str());
  }
  std::string result = hosts_[active_idx_].provider->chat_stream(messages, on_token);
  if (result.empty() && failover()) {
    if (Config::instance().trace) {
      stderr_trace->log("[TRACE] MultiHost: failover → %s\n", hosts_[active_idx_].provider->host().c_str());
    }
    result = hosts_[active_idx_].provider->chat_stream(messages, on_token);
  }
  return result;
}

std::vector<std::string> MultiHostProvider::list_models() {
  // Aggregate models from all healthy hosts (deduplicated)
  std::vector<std::string> all;
  for (const auto& h : hosts_) {
    if (!h.healthy) {
      continue;
    }
    for (const auto& m : h.models) {
      if (std::find(all.begin(), all.end(), m) == all.end()) {
        all.push_back(m);
      }
    }
  }
  return all;
}

std::vector<ModelInfo> MultiHostProvider::get_model_info() {
  // Aggregate from all healthy hosts
  std::vector<ModelInfo> all;
  for (auto& h : hosts_) {
    if (!h.healthy) {
      continue;
    }
    auto infos = h.provider->get_model_info();
    all.insert(all.end(), infos.begin(), infos.end());
  }
  return all;
}

bool MultiHostProvider::is_model_running(const std::string& model_name) {
  return hosts_[active_idx_].provider->is_model_running(model_name);
}

std::string MultiHostProvider::name() const { return "multi-ollama"; }

std::string MultiHostProvider::host() const { return hosts_[active_idx_].provider->host(); }
