/**
 * @file prompt_template.h
 * @brief Template rendering with {{variable}} substitution (ADR-096).
 */

#ifndef ORCHESTRATOR_PROMPT_TEMPLATE_H
#define ORCHESTRATOR_PROMPT_TEMPLATE_H

#include <string>
#include <unordered_map>

/// Render a template string, replacing {{key}} with values from the map.
/// Unknown keys are left as-is.
std::string render_template(const std::string& tmpl, const std::unordered_map<std::string, std::string>& vars);

/// Load a prompt template from res/agents/prompts/<name>.txt
/// Returns empty string if file not found.
std::string load_prompt(const std::string& agent_name);

#endif  // ORCHESTRATOR_PROMPT_TEMPLATE_H
