/**
 * @file planner.h
 * @brief Smart routing — classifies prompt complexity and picks the best host/model.
 *
 * Heuristic-based (no LLM call for classification):
 *   simple  → small model on fast host (jarvis/pepper 3B)
 *   medium  → mid model (qwen-coder 14B on <hostname>)
 *   complex → large model (27B+) or cloud fallback
 */

#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include <vector>

/// Complexity tier for routing decisions.
enum class Complexity { Simple, Medium, Complex };

/// Classify a user prompt by complexity using string heuristics.
Complexity classify_prompt(const std::string& prompt);

/// Route target: which host and model to use.
struct RouteTarget {
  std::string host;    ///< "host:port"
  std::string model;   ///< Model name
  std::string reason;  ///< Why this route was chosen
};

/// Pick the best host/model for a given complexity level.
/// Uses the configured hosts and their known models.
RouteTarget pick_route(Complexity level, const std::vector<std::string>& hosts);

/// Convenience: classify + route in one call.
RouteTarget plan_route(const std::string& prompt, const std::vector<std::string>& hosts);

#endif  // PLANNER_H
