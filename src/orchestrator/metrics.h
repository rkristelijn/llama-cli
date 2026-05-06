/**
 * @file metrics.h
 * @brief Per-model LLM performance metrics (ADR-096).
 *
 * Tracks calls, tokens, speed, and success rate per model+agent combination.
 * Persisted to ~/.llama-cli/metrics.json. Used by routing rule engine.
 */

#ifndef ORCHESTRATOR_METRICS_H
#define ORCHESTRATOR_METRICS_H

#include <string>

/// Record a completed LLM call in the metrics store.
/// Called after every LLM response (success or failure).
void metrics_record(const std::string& model, const std::string& host, const std::string& agent, int duration_ms, int tokens_prompt,
                    int tokens_completion, bool success);

/// Get the routing score for a model+agent combination.
/// Returns 0.0 if no data available (cold start).
/// score = avg_tokens_per_sec × success_rate × recency_factor
double metrics_score(const std::string& model, const std::string& agent);

/// Get the metrics file path.
std::string metrics_path();

#endif  // ORCHESTRATOR_METRICS_H
