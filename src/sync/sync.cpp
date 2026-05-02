/**
 * @file sync.cpp
 * @brief Pure helper functions for sync-mode annotation processing (ADR-056).
 */

#include "sync/sync.h"

#include <climits>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

/// Check if a capability is enabled in the comma-separated capabilities string
/// Check if a capability string contains a specific capability.
/// Capabilities are comma-separated: "read,write,exec".
/// Exact match required — "read" does not match "readonly".
bool has_cap(const std::string& caps, const std::string& cap) {
  size_t pos = 0;
  while ((pos = caps.find(cap, pos)) != std::string::npos) {
    bool at_start = (pos == 0 || caps[pos - 1] == ',');
    bool at_end = (pos + cap.size() >= caps.size() || caps[pos + cap.size()] == ',');
    if (at_start && at_end) {
      return true;
    }
    pos += cap.size();
  }
  return false;
}

/// Commands safe to auto-execute with the "read" capability.
/// These cannot modify files, network, or system state.
/// Used by is_read_only() to validate each segment of a piped command.
static const std::vector<std::string> read_only_commands = {"cat",  "ls",   "find", "grep",     "head",    "tail",    "wc",   "stat",
                                                            "tree", "du",   "df",   "file",     "diff",    "sort",    "uniq", "awk",
                                                            "sed",  "less", "more", "realpath", "dirname", "basename"};

/// Check if a command is read-only (safe to run with "read" capability).
/// Splits on pipe segments and validates each command's first word.
/// Blocks any command containing > (redirect = potential file modification).
bool is_read_only(const std::string& cmd) {
  // Block redirects — any > means potential file modification
  if (cmd.find('>') != std::string::npos) {
    return false;
  }
  // Check each pipe segment independently
  std::istringstream segments(cmd);
  std::string segment;
  while (std::getline(segments, segment, '|')) {
    size_t start = segment.find_first_not_of(" \t");
    if (start == std::string::npos) {
      continue;
    }
    size_t end = segment.find_first_of(" \t", start);
    std::string word = segment.substr(start, end - start);
    bool found = false;
    for (const auto& safe : read_only_commands) {
      if (word == safe) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

/// Parse <exec>cmd</exec> tags from LLM response text.
/// Returns a vector of command strings to be executed.
/// Used by both sync mode (main.cpp) and REPL mode (repl.cpp).
std::vector<std::string> parse_exec_tags(const std::string& text) {
  std::vector<std::string> cmds;
  const std::string open = "<exec>";
  const std::string close = "</exec>";
  size_t pos = 0;
  while ((pos = text.find(open, pos)) != std::string::npos) {
    size_t start = pos + open.size();
    size_t end = text.find(close, start);
    if (end == std::string::npos) {
      break;
    }
    cmds.push_back(text.substr(start, end - start));
    pos = end + close.size();
  }
  return cmds;
}

/// Check if a file path is within the sandbox directory.
/// Resolves relative paths and checks prefix match. Prevents path traversal.
/// For new files (not yet on disk), validates the parent directory instead.
bool path_allowed(const std::string& path, const std::string& sandbox) {
  // Resolve sandbox to absolute path
  char sandbox_real[PATH_MAX];
  if (!realpath(sandbox.c_str(), sandbox_real)) {
    return false;
  }
  std::string sandbox_prefix(sandbox_real);
  sandbox_prefix += "/";

  char path_real[PATH_MAX];
  if (realpath(path.c_str(), path_real)) {
    std::string resolved(path_real);
    return resolved == sandbox_real || resolved.rfind(sandbox_prefix, 0) == 0;
  }

  size_t slash = path.rfind('/');
  std::string parent = (slash != std::string::npos) ? path.substr(0, slash) : ".";
  if (!realpath(parent.c_str(), path_real)) {
    return false;
  }
  std::string resolved(path_real);
  return resolved == sandbox_real || resolved.rfind(sandbox_prefix, 0) == 0;
}

/// Extract resource file paths from a .kiro agent JSON config.
/// Parses "resources": ["file://path", ...] and strips the file:// prefix.
std::vector<std::string> extract_resources(const std::string& json) {
  std::vector<std::string> paths;
  const std::string prefix = "\"file://";
  size_t pos = 0;
  while ((pos = json.find(prefix, pos)) != std::string::npos) {
    size_t start = pos + prefix.size();
    size_t end = json.find('"', start);
    if (end != std::string::npos) {
      paths.push_back(json.substr(start, end - start));
    }
    pos = (end != std::string::npos) ? end + 1 : start;
  }
  return paths;
}
