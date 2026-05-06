/**
 * @file agent_config.h
 * @brief Agent configuration with per-agent permissions (ADR-096 Phase 3).
 *
 * Defines agent roles (primary/subagent) and tool permissions (allow/ask/deny).
 * Loaded from res/agents/agents.yml or ~/.llama-cli/agents.yml.
 */

#ifndef ORCHESTRATOR_AGENT_CONFIG_H
#define ORCHESTRATOR_AGENT_CONFIG_H

#include <string>
#include <unordered_map>
#include <vector>

/// Permission level for a tool action.
enum class Permission { Allow, Ask, Deny };

/// Agent mode — primary (user-switchable) or subagent (invoked via at-mention).
enum class AgentMode { Primary, Subagent };

/// Configuration for a single agent.
struct AgentConfig {
  std::string name;                     ///< Agent identifier (e.g., "build", "plan")
  std::string description;              ///< One-line description
  AgentMode mode = AgentMode::Primary;  ///< Primary or subagent
  std::string model;                    ///< Preferred model (empty = use default)
  std::string prompt_file;              ///< Prompt template filename (e.g., "plan.txt")
  double temperature = 0.7;             ///< LLM temperature
  /// Per-tool permissions: key = tool name (read/write/str_replace/exec)
  std::unordered_map<std::string, Permission> permissions;
};

/// Load agent configurations from YAML file.
std::vector<AgentConfig> load_agent_configs();

/// Find an agent config by name. Returns nullptr if not found.
const AgentConfig* find_agent_config(const std::vector<AgentConfig>& agents, const std::string& name);

/// Check if a tool action is permitted for the active agent.
/// Returns the permission level for the given tool.
Permission check_permission(const AgentConfig& agent, const std::string& tool);

/// Get the currently active agent name (thread-local state).
const std::string& active_agent_name();

/// Set the active agent (called by /agent command).
void set_active_agent(const std::string& name);

#endif  // ORCHESTRATOR_AGENT_CONFIG_H
