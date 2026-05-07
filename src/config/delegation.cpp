/**
 * @file delegation.cpp
 * @brief Role-based delegation router (ADR-100).
 *
 * Parses .config/delegation.yml (simple key: value format).
 * Maps role types to model\@host:port targets.
 */

#include "config/delegation.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

/// Parse "model@host:port" or just "model" into components.
static DelegationTarget parse_target(const std::string& value) {
  DelegationTarget target;
  auto at = value.find('@');
  if (at != std::string::npos) {
    // model@host:port
    target.model = value.substr(0, at);
    std::string host_port = value.substr(at + 1);
    auto colon = host_port.find(':');
    if (colon != std::string::npos) {
      target.host = host_port.substr(0, colon);
      target.port = host_port.substr(colon + 1);
    } else {
      target.host = host_port;
    }
  } else {
    // Just model name — use current host
    target.model = value;
  }
  return target;
}

/// Load delegation config from file. Simple YAML subset: "  key: value"
static std::unordered_map<std::string, DelegationTarget> load_config() {
  std::unordered_map<std::string, DelegationTarget> roles;
  // Try local override first (gitignored, has hostnames), then shared config
  std::ifstream f(".config/delegation.local.yml");
  if (!f.is_open()) {
    f.open(".config/delegation.yml");
  }
  if (!f.is_open()) {
    const char* home = std::getenv("HOME");
    if (home) {
      f.open(std::string(home) + "/.llama-cli/delegation.yml");
    }
  }
  if (!f.is_open()) {
    return roles;
  }
  std::string line;
  bool in_roles = false;
  while (std::getline(f, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') {
      continue;
    }
    // Detect "roles:" section
    if (line == "roles:" || line == "roles: ") {
      in_roles = true;
      continue;
    }
    // Parse indented "  key: value" lines under roles:
    if (in_roles && line.size() > 2 && line[0] == ' ') {
      auto colon = line.find(':');
      if (colon == std::string::npos) {
        continue;
      }
      std::string key = line.substr(0, colon);
      // Trim leading whitespace from key
      while (!key.empty() && key.front() == ' ') {
        key.erase(key.begin());
      }
      std::string val = line.substr(colon + 1);
      // Trim whitespace and quotes from value
      while (!val.empty() && (val.front() == ' ' || val.front() == '"')) {
        val.erase(val.begin());
      }
      while (!val.empty() && (val.back() == '"' || val.back() == ' ')) {
        val.pop_back();
      }
      if (!key.empty() && !val.empty()) {
        roles[key] = parse_target(val);
      }
    }
  }
  return roles;
}

DelegationTarget resolve_delegation(const std::string& type) {
  // Cache config on first call (loaded once per process)
  static auto roles = load_config();
  auto it = roles.find(type);
  if (it != roles.end()) {
    return it->second;
  }
  // Unknown type — return empty (caller uses current config as fallback)
  return {};
}
