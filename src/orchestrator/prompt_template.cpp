/**
 * @file prompt_template.cpp
 * @brief Template rendering implementation (ADR-096).
 */

#include "orchestrator/prompt_template.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "trace/trace.h"

/// Render a template by replacing {{key}} placeholders with values from vars
/// map. Scans left-to-right for {{ and }} delimiters. Unknown keys are
/// preserved verbatim (not removed).
std::string render_template(const std::string& tmpl, const std::unordered_map<std::string, std::string>& vars) {
  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] render_template: %zu vars, template=%zu chars\n", vars.size(), tmpl.size());
  }
  std::string result;
  size_t pos = 0;
  while (pos < tmpl.size()) {
    size_t start = tmpl.find("{{", pos);
    if (start == std::string::npos) {
      result += tmpl.substr(pos);
      break;
    }
    result += tmpl.substr(pos, start - pos);
    size_t end = tmpl.find("}}", start + 2);
    if (end == std::string::npos) {
      result += tmpl.substr(start);
      break;
    }
    std::string key = tmpl.substr(start + 2, end - start - 2);
    auto it = vars.find(key);
    if (it != vars.end()) {
      result += it->second;
    } else {
      result += "{{" + key + "}}";  // leave unknown keys
    }
    pos = end + 2;
  }
  return result;
}

/// Load a prompt template file for the given agent.
/// Search order: res/agents/prompts/ (bundled), ~/.llama-cli/prompts/ (user
/// override). Returns empty string if no template found for this agent.
std::string load_prompt(const std::string& agent_name) {
  // Try bundled path first, then installed path
  std::string paths[] = {"res/agents/prompts/" + agent_name + ".txt",
                         std::string(std::getenv("HOME") ? std::getenv("HOME") : ".") + "/.llama-cli/prompts/" + agent_name + ".txt"};
  for (const auto& path : paths) {
    std::ifstream f(path);
    if (f.is_open()) {
      std::ostringstream ss;
      ss << f.rdbuf();
      if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
        stderr_trace->log("[TRACE] load_prompt: agent=%s path=%s size=%zu\n", agent_name.c_str(), path.c_str(), ss.str().size());
      }
      return ss.str();
    }
  }
  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] load_prompt: agent=%s NOT FOUND\n", agent_name.c_str());
  }
  return "";
}
