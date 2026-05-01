/**
 * @file sync.h
 * @brief Pure helper functions for sync-mode annotation processing (ADR-056).
 */

#ifndef SYNC_H
#define SYNC_H

#include <string>
#include <vector>

/// Check if a capability is enabled in a comma-separated capabilities string.
bool has_cap(const std::string& caps, const std::string& cap);

/// Check if a shell command is read-only (all pipe segments in allowlist, no redirects).
bool is_read_only(const std::string& cmd);

/// Parse <exec>cmd</exec> tags from LLM response text.
std::vector<std::string> parse_exec_tags(const std::string& text);

/// Check if a file path is within the sandbox directory.
/// Resolves both paths to absolute form to catch ".." traversal.
bool path_allowed(const std::string& path, const std::string& sandbox);

/// Extract "file://path" entries from a JSON resources array.
std::vector<std::string> extract_resources(const std::string& json);

#endif  // SYNC_H
