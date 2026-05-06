/**
 * @file orchestrator.h
 * @brief Multi-agent orchestrator (ADR-096).
 *
 * Routes user requests: Simple → direct LLM, Complex → Planner → Executor →
 * Reviewer loop. Phase 1: skeleton that always routes to the existing
 * single-LLM path.
 */

#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <string>
#include <vector>

#include "ollama/ollama.h"
#include "orchestrator/task.h"
#include "planner/planner.h"

/// Result of orchestrator handling a user request.
struct OrchestratorResult {
  bool used_plan = false;  ///< True if multi-agent path was used
  TaskPlan plan;           ///< The plan (empty if single-shot)
  std::string response;    ///< Final response text
};

/// Orchestrator decides how to handle a user request.
/// Phase 1: always returns single-shot (used_plan=false).
/// Phase 2+: complex prompts get decomposed into plans.
OrchestratorResult orchestrate(const std::string& prompt, const std::vector<Message>& history);

/// Check if a prompt should use multi-agent orchestration.
/// Currently: always false (Phase 1 skeleton).
bool should_orchestrate(const std::string& prompt);

#endif  // ORCHESTRATOR_H
