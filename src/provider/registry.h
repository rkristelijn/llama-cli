/**
 * @file registry.h
 * @brief Unified Provider→Host→Model registry (ADR-081).
 *
 * Single source of truth for all available LLM endpoints.
 * Populated at startup via scan, queryable by /model and /provider commands.
 */

#ifndef REGISTRY_H
#define REGISTRY_H

#include <string>
#include <vector>

/// Cost tier for routing decisions.
enum class CostTier { Free, Cheap, Expensive };

/// Model capabilities for smart routing.
enum class Capability { Code, Vision, General, Reasoning, CurrentInfo };

/// A single model entry in the registry.
struct ModelEntry {
  std::string name;                      ///< Model name (e.g., "qwen2.5-coder:14b")
  std::string provider;                  ///< Provider name (e.g., "ollama")
  std::string host;                      ///< Host address (e.g., "apsnlmac4050:11434")
  double tokens_per_sec = 0;             ///< Benchmark speed (0 = unknown)
  double params_b = 0;                   ///< Parameter count in billions
  CostTier cost = CostTier::Free;        ///< Cost tier
  std::vector<Capability> capabilities;  ///< What this model is good at
  bool available = true;                 ///< Currently reachable
};

/// The unified registry — all models across all providers and hosts.
struct ModelRegistry {
  std::vector<ModelEntry> models;  ///< All discovered models across providers.

  /// Find all models matching a capability.
  std::vector<const ModelEntry*> by_capability(Capability cap) const;

  /// Find the fastest model for a given capability.
  const ModelEntry* fastest(Capability cap) const;

  /// Find by exact name (first match).
  const ModelEntry* by_name(const std::string& name) const;

  /// Count unique providers.
  int provider_count() const;

  /// Count unique hosts.
  int host_count() const;

  /// Get unique provider names.
  std::vector<std::string> providers() const;
};

/// Scan all configured providers/hosts and build the registry.
/// Probes Ollama hosts, checks tgpt/gemini/kiro availability.
ModelRegistry scan_all_providers();

#endif  // REGISTRY_H
