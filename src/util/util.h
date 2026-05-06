/**
 * @file util.h
 * @brief Shared utility functions (URL encoding, color mapping).
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>

/// URL-encode a string (spaces → +, special chars → %XX).
std::string url_encode(const std::string& input);

/// Map a color name (e.g. "red", "purple") to its ANSI escape code.
std::string color_name_to_ansi(const std::string& name);

/// Map an ANSI escape code back to a color name.
std::string ansi_to_name(const std::string& code);

/// Resolve an IP:port string to a friendly display name (hostname:port).
/// Uses lazy DNS reverse lookup with caching. Returns original if unresolvable.
/// Example: "10.0.0.72:11434" → "jarvis.local:11434"
std::string resolve_display_name(const std::string& host_port);

/// Shell-escape a string with single quotes (safe for subprocess invocation).
std::string shell_escape(const std::string& s);

/// Strip ANSI escape sequences from a string (for cleaning subprocess output).
std::string strip_ansi(const std::string& s);

#endif  // UTIL_H
