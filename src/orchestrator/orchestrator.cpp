/**
 * @file orchestrator.cpp
 * @brief Multi-agent orchestrator skeleton (ADR-096, Phase 1).
 *
 * Currently a pass-through: all prompts go to the existing single-LLM path.
 * Phase 2+ will add planner/executor/reviewer routing for complex prompts.
 */

#include "orchestrator/orchestrator.h"

#include "trace/trace.h"

bool should_orchestrate(const std::string& prompt) {
  (void)prompt;
  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] should_orchestrate: false (Phase 1 skeleton)\n");
  }
  return false;
}

OrchestratorResult orchestrate(const std::string& prompt, const std::vector<Message>& history) {
  (void)history;
  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] orchestrate: pass-through prompt=%zu chars\n", prompt.size());
  }
  OrchestratorResult result;
  result.used_plan = false;
  result.plan.goal = prompt;
  return result;
}

// --- Orchestrator design notes ---
// Phase 1: pass-through skeleton (all prompts go to existing single-LLM path).
// Phase 2+: complex prompts get decomposed into plans by the plan agent.
// The orchestrator is deterministic code, not an LLM — it makes routing decisions
// based on prompt complexity, active agent, and metrics scores.
// Future: rate limiting, load balancing, and predictive model selection.
