/**
 * @file agent.h
 * @brief Agent persona loading and switching.
 *
 * Loads persona definitions from res/agents/personas.yml (bundled)
 * or ~/.llama-cli/agents/personas.yml (user override).
 */

#ifndef AGENT_H
#define AGENT_H

#include <string>
#include <vector>

/// An agent persona — personality overlay for the LLM.
struct AgentPersona {
  std::string name;           ///< Short identifier (e.g., "bofh")
  std::string description;    ///< One-line description
  std::string system_prompt;  ///< System prompt override
  double temperature = 0.7;   ///< Suggested temperature
};

/// Parse personas from YAML content (minimal parser, no library needed).
std::vector<AgentPersona> parse_personas(const std::string& yaml_content);

/// Load personas from bundled file or user override.
std::vector<AgentPersona> load_personas();

/// Find a persona by name (case-insensitive). Returns nullptr if not found.
const AgentPersona* find_persona(const std::vector<AgentPersona>& personas, const std::string& name);

#endif  // AGENT_H
