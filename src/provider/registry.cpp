/**
 * @file registry.cpp
 * @brief Scans all providers and builds the unified model registry (ADR-081).
 */

#include "provider/registry.h"

#include <algorithm>
#include <set>

#include "config/config.h"
#include "exec/exec.h"
#include "ollama/ollama.h"

// --- ModelRegistry methods ---

std::vector<const ModelEntry*> ModelRegistry::by_capability(Capability cap) const {
  std::vector<const ModelEntry*> result;
  for (const auto& m : models) {
    if (!m.available) {
      continue;
    }
    for (auto c : m.capabilities) {
      if (c == cap) {
        result.push_back(&m);
        break;
      }
    }
  }
  return result;
}

const ModelEntry* ModelRegistry::fastest(Capability cap) const {
  const ModelEntry* best = nullptr;
  for (const auto& m : models) {
    if (!m.available) {
      continue;
    }
    for (auto c : m.capabilities) {
      if (c == cap) {
        if (!best || m.tokens_per_sec > best->tokens_per_sec) {
          best = &m;
        }
        break;
      }
    }
  }
  return best;
}

const ModelEntry* ModelRegistry::by_name(const std::string& name) const {
  for (const auto& m : models) {
    if (m.name == name) {
      return &m;
    }
  }
  return nullptr;
}

int ModelRegistry::provider_count() const {
  std::set<std::string> seen;
  for (const auto& m : models) {
    seen.insert(m.provider);
  }
  return static_cast<int>(seen.size());
}

int ModelRegistry::host_count() const {
  std::set<std::string> seen;
  for (const auto& m : models) {
    seen.insert(m.host);
  }
  return static_cast<int>(seen.size());
}

std::vector<std::string> ModelRegistry::providers() const {
  std::vector<std::string> result;
  std::set<std::string> seen;
  for (const auto& m : models) {
    if (seen.insert(m.provider).second) {
      result.push_back(m.provider);
    }
  }
  return result;
}

/// Infer capabilities from model name/family.
static std::vector<Capability> infer_capabilities(const std::string& name, double params) {
  std::vector<Capability> caps = {Capability::General};
  // Code models
  if (name.find("coder") != std::string::npos || name.find("deepseek") != std::string::npos) {
    caps.push_back(Capability::Code);
  }
  // Vision models
  if (name.find("llava") != std::string::npos || name.find("gemma4") != std::string::npos) {
    caps.push_back(Capability::Vision);
  }
  // Reasoning (larger models)
  if (params >= 20.0) {
    caps.push_back(Capability::Reasoning);
  }
  return caps;
}

/// Estimate tokens/sec from parameter size (rough heuristic when no benchmark data).
static double estimate_speed(double params_b, const std::string& host) {
  // Rough estimates based on typical hardware
  if (host.find("localhost") != std::string::npos) {
    // Likely weaker hardware
    if (params_b <= 3) {
      return 20.0;
    }
    if (params_b <= 7) {
      return 10.0;
    }
    return 5.0;
  }
  // Remote host (assumed decent GPU)
  if (params_b <= 3) {
    return 85.0;
  }
  if (params_b <= 14) {
    return 42.0;
  }
  if (params_b <= 27) {
    return 8.0;
  }
  return 3.0;
}

ModelRegistry scan_all_providers() {
  ModelRegistry reg;
  const auto& cfg = Config::instance();

  // --- Ollama hosts ---
  std::vector<std::string> hosts = cfg.hosts;
  if (hosts.empty()) {
    hosts.push_back(cfg.host + ":" + cfg.port);
  }

  for (const auto& hp : hosts) {
    Config tmp;
    auto colon = hp.find(':');
    tmp.host = (colon != std::string::npos) ? hp.substr(0, colon) : hp;
    tmp.port = (colon != std::string::npos) ? hp.substr(colon + 1) : "11434";
    tmp.timeout = 3;

    auto infos = get_model_info(tmp);
    for (const auto& info : infos) {
      // Skip embedding models
      if (info.name.find("embed") != std::string::npos) {
        continue;
      }
      double params = 0;
      try {
        params = std::stod(info.params);
      } catch (...) {
      }

      ModelEntry entry;
      entry.name = info.name;
      entry.provider = "ollama";
      entry.host = hp;
      entry.params_b = params;
      entry.tokens_per_sec = estimate_speed(params, hp);
      entry.cost = CostTier::Free;
      entry.capabilities = infer_capabilities(info.name, params);
      entry.available = true;
      reg.models.push_back(entry);
    }
  }

  // --- tgpt ---
  ExecResult tgpt_check = cmd_exec("which tgpt", 3, 200);
  if (tgpt_check.exit_code == 0) {
    ModelEntry entry;
    entry.name = "tgpt-default";
    entry.provider = "tgpt";
    entry.host = "cloud";
    entry.tokens_per_sec = 0;
    entry.cost = CostTier::Free;
    entry.capabilities = {Capability::General, Capability::CurrentInfo};
    entry.available = true;
    reg.models.push_back(entry);
  }

  // --- gemini ---
  ExecResult gemini_check = cmd_exec("which gemini", 3, 200);
  if (gemini_check.exit_code == 0) {
    ModelEntry entry;
    entry.name = "gemini-cli";
    entry.provider = "gemini";
    entry.host = "cloud";
    entry.tokens_per_sec = 0;
    entry.cost = CostTier::Free;
    entry.capabilities = {Capability::General, Capability::Code, Capability::Reasoning};
    entry.available = true;
    reg.models.push_back(entry);
  }

  // --- kiro-cli ---
  ExecResult kiro_check = cmd_exec("which kiro-cli-chat", 3, 200);
  if (kiro_check.exit_code == 0) {
    // Parse model list from kiro-cli-chat for real model names + costs
    ExecResult kiro_models = cmd_exec("kiro-cli-chat chat --list-models -f json", 10, 50000);
    if (kiro_models.exit_code == 0) {
      size_t pos = 0;
      while ((pos = kiro_models.output.find("\"model_name\":\"", pos)) != std::string::npos) {
        pos += 14;
        auto end = kiro_models.output.find("\"", pos);
        std::string name = kiro_models.output.substr(pos, end - pos);
        // Extract rate multiplier
        double rate = 1.0;
        auto rate_pos = kiro_models.output.find("\"rate_multiplier\":", pos);
        if (rate_pos != std::string::npos && rate_pos < pos + 300) {
          try {
            rate = std::stod(kiro_models.output.substr(rate_pos + 18, 10));
          } catch (...) {
          }
        }
        ModelEntry entry;
        entry.name = name;
        entry.provider = "kiro-cli";
        entry.host = "cloud";
        entry.tokens_per_sec = 0;
        entry.cost = (rate > 1.5) ? CostTier::Expensive : (rate > 0.5 ? CostTier::Cheap : CostTier::Free);
        entry.capabilities = {Capability::General, Capability::Code, Capability::Reasoning};
        entry.available = true;
        reg.models.push_back(entry);
        pos = end;
      }
    } else {
      // Fallback: just register one generic entry
      ModelEntry entry;
      entry.name = "kiro-auto";
      entry.provider = "kiro-cli";
      entry.host = "cloud";
      entry.cost = CostTier::Expensive;
      entry.capabilities = {Capability::General, Capability::Code, Capability::Reasoning};
      entry.available = true;
      reg.models.push_back(entry);
    }
  }

  // Sort: fastest first within each provider
  std::sort(reg.models.begin(), reg.models.end(),
            [](const ModelEntry& a, const ModelEntry& b) { return a.tokens_per_sec > b.tokens_per_sec; });

  return reg;
}
