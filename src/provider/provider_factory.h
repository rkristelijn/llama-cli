/**
 * @file provider_factory.h
 * @brief Factory for creating LLMProvider instances based on config (ADR-020).
 *
 * Centralizes provider creation logic. Supports "ollama" (default) and "mock".
 * Future: "openai", "tgpt", etc.
 */

#ifndef PROVIDER_FACTORY_H
#define PROVIDER_FACTORY_H

#include <memory>

#include "config/config.h"
#include "provider/provider.h"

/// Create a provider instance based on config.provider field.
/// Returns OllamaProvider for "ollama", MockProvider for "mock".
/// Throws std::runtime_error for unknown provider names.
std::unique_ptr<LLMProvider> create_provider(const Config& cfg);

#endif  // PROVIDER_FACTORY_H
