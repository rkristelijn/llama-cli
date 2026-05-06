/**
 * @file agent_config.cpp
 * @brief Agent config loading and permission checking (ADR-096 Phase 3).
 */

#include "orchestrator/agent_config.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "trace/trace.h"

/// Currently active agent name. Defaults to "build" (full access).
static std::string g_active_agent = "build";

const std::string& active_agent_name() { return g_active_agent; }

void set_active_agent(const std::string& name) { g_active_agent = name; }

/// Parse permission string to enum.
static Permission parse_permission(const std::string& s) {
  if (s == "deny") return Permission::Deny;
  if (s == "ask") return Permission::Ask;
  return Permission::Allow;
}

/// Built-in agent definitions (used if no agents.yml found).
static std::vector<AgentConfig> builtin_agents() {
  return {
      {"build",
       "Default agent — full tool access",
       AgentMode::Primary,
       "",
       "build.txt",
       0.7,
       {{"read", Permission::Allow}, {"write", Permission::Allow}, {"str_replace", Permission::Allow}, {"exec", Permission::Allow}}},
      {"plan",
       "Read-only analysis and planning",
       AgentMode::Primary,
       "",
       "plan.txt",
       0.3,
       {{"read", Permission::Allow}, {"write", Permission::Deny}, {"str_replace", Permission::Deny}, {"exec", Permission::Deny}}},
      {"general",
       "Multi-step task execution",
       AgentMode::Subagent,
       "",
       "general.txt",
       0.5,
       {{"read", Permission::Allow}, {"write", Permission::Allow}, {"str_replace", Permission::Allow}, {"exec", Permission::Ask}}},
      {"explore",
       "Read-only codebase exploration",
       AgentMode::Subagent,
       "",
       "explore.txt",
       0.3,
       {{"read", Permission::Allow}, {"write", Permission::Deny}, {"str_replace", Permission::Deny}, {"exec", Permission::Deny}}},
      {"reviewer",
       "Validate output from other agents",
       AgentMode::Subagent,
       "",
       "reviewer.txt",
       0.1,
       {{"read", Permission::Allow}, {"write", Permission::Deny}, {"str_replace", Permission::Deny}, {"exec", Permission::Deny}}},
  };
}

std::vector<AgentConfig> load_agent_configs() {
  // Try loading from file, fall back to builtins
  std::string paths[] = {"res/agents/agents.yml", std::string(std::getenv("HOME") ? std::getenv("HOME") : ".") + "/.llama-cli/agents.yml"};

  for (const auto& path : paths) {
    std::ifstream f(path);
    if (!f.is_open()) continue;

    // Minimal YAML parser for agent configs
    std::vector<AgentConfig> agents;
    AgentConfig current;
    std::string line;
    bool in_agent = false;

    while (std::getline(f, line)) {
      if (line.empty() || line[0] == '#') continue;

      if (line.find("- name:") == 0 || line.find("- name:") != std::string::npos) {
        if (in_agent) agents.push_back(current);
        current = AgentConfig{};
        current.name = line.substr(line.find(':') + 2);
        in_agent = true;
      } else if (in_agent) {
        // Strip leading whitespace
        size_t indent = line.find_first_not_of(" \t");
        if (indent == std::string::npos) continue;
        std::string trimmed = line.substr(indent);

        if (trimmed.find("description:") == 0) {
          current.description = trimmed.substr(13);
        } else if (trimmed.find("mode:") == 0) {
          std::string m = trimmed.substr(6);
          current.mode = (m == "subagent") ? AgentMode::Subagent : AgentMode::Primary;
        } else if (trimmed.find("model:") == 0) {
          current.model = trimmed.substr(7);
        } else if (trimmed.find("prompt:") == 0) {
          current.prompt_file = trimmed.substr(8);
        } else if (trimmed.find("temperature:") == 0) {
          current.temperature = std::stod(trimmed.substr(13));
        } else if (trimmed.find("read:") == 0) {
          current.permissions["read"] = parse_permission(trimmed.substr(6));
        } else if (trimmed.find("write:") == 0) {
          current.permissions["write"] = parse_permission(trimmed.substr(7));
        } else if (trimmed.find("str_replace:") == 0) {
          current.permissions["str_replace"] = parse_permission(trimmed.substr(13));
        } else if (trimmed.find("exec:") == 0) {
          current.permissions["exec"] = parse_permission(trimmed.substr(6));
        }
      }
    }
    if (in_agent) agents.push_back(current);

    if (!agents.empty()) {
      if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
        stderr_trace->log("[TRACE] load_agent_configs: loaded %zu agents from %s\n", agents.size(), path.c_str());
      }
      return agents;
    }
  }

  // Fallback to builtins
  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] load_agent_configs: using 5 built-in agents\n");
  }
  return builtin_agents();
}

const AgentConfig* find_agent_config(const std::vector<AgentConfig>& agents, const std::string& name) {
  for (const auto& a : agents) {
    if (a.name == name) return &a;
  }
  return nullptr;
}

Permission check_permission(const AgentConfig& agent, const std::string& tool) {
  auto it = agent.permissions.find(tool);
  if (it != agent.permissions.end()) return it->second;
  // Default: allow for primary, ask for subagent
  return (agent.mode == AgentMode::Primary) ? Permission::Allow : Permission::Ask;
}

// --- Permission system notes ---
// The permission model follows OpenCode's allow/ask/deny pattern (ADR-096).
// Primary agents default to "allow" for unspecified tools.
// Subagents default to "ask" for unspecified tools.
// The active agent is tracked globally and checked by annotation handlers.
// Permission checks happen at the point of action (write/exec), not at prompt time.
// This ensures the LLM can still suggest actions that get blocked at execution.
