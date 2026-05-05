/**
 * @file agent.cpp
 * @brief Agent persona loading — minimal YAML parser for flat persona list.
 */

#include "agent/agent.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

/// Trim leading/trailing whitespace from a string.
static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

/// Extract value after "key: value" — strips leading space.
static std::string extract_value(const std::string& line, const std::string& key) {
  auto pos = line.find(key + ":");
  if (pos == std::string::npos) return "";
  return trim(line.substr(pos + key.size() + 1));
}

std::vector<AgentPersona> parse_personas(const std::string& yaml_content) {
  std::vector<AgentPersona> personas;
  std::istringstream stream(yaml_content);
  std::string line;
  AgentPersona current;
  bool in_multiline = false;
  bool has_current = false;

  while (std::getline(stream, line)) {
    // Skip comments and empty lines at top level
    if (!in_multiline && (line.empty() || line[0] == '#')) continue;

    // New list item starts with "- name:"
    if (line.find("- name:") != std::string::npos) {
      if (has_current) {
        personas.push_back(current);
      }
      current = {};
      current.name = extract_value(line, "- name");
      has_current = true;
      in_multiline = false;
      continue;
    }

    // Inside multiline block: lines indented with 4+ spaces
    if (in_multiline) {
      if (line.size() >= 4 && line[0] == ' ' && line[1] == ' ' && line[2] == ' ' && line[3] == ' ') {
        current.system_prompt += line.substr(4) + "\n";
        continue;
      }
      // End of multiline block
      in_multiline = false;
    }

    // Key-value pairs (indented with 2 spaces)
    std::string trimmed = trim(line);
    if (trimmed.find("description:") == 0) {
      current.description = extract_value(trimmed, "description");
    } else if (trimmed.find("system_prompt:") == 0) {
      // Check for | (multiline indicator)
      std::string val = extract_value(trimmed, "system_prompt");
      if (val == "|") {
        in_multiline = true;
        current.system_prompt = "";
      } else {
        current.system_prompt = val;
      }
    } else if (trimmed.find("temperature:") == 0) {
      std::string val = extract_value(trimmed, "temperature");
      try {
        current.temperature = std::stod(val);
      } catch (...) {
        current.temperature = 0.7;
      }
    }
  }
  // Don't forget the last persona
  if (has_current) {
    personas.push_back(current);
  }
  return personas;
}

/// Read file content or return empty string.
static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) return "";
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::vector<AgentPersona> load_personas() {
  // User override takes priority
  const char* home = std::getenv("HOME");
  if (home) {
    std::string user_path = std::string(home) + "/.llama-cli/agents/personas.yml";
    std::string content = read_file(user_path);
    if (!content.empty()) {
      return parse_personas(content);
    }
  }
  // Bundled fallback (relative to binary or repo root)
  std::string content = read_file("res/agents/personas.yml");
  if (!content.empty()) {
    return parse_personas(content);
  }
  return {};
}

const AgentPersona* find_persona(const std::vector<AgentPersona>& personas, const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  for (const auto& p : personas) {
    std::string pname = p.name;
    std::transform(pname.begin(), pname.end(), pname.begin(), [](unsigned char c) { return std::tolower(c); });
    if (pname == lower) return &p;
  }
  return nullptr;
}
