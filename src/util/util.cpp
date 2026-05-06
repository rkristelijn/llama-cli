/**
 * @file util.cpp
 * @brief Shared utility functions (URL encoding, color mapping).
 */

#include "util/util.h"

#include <cctype>
#include <cstdio>
#include <string>

/// URL-encode a string (spaces → +, special chars → %XX).
std::string url_encode(const std::string& input) {
  std::string out;
  for (char c : input) {
    if (c == ' ') {
      out += '+';
    } else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      out += buf;
    }
  }
  return out;
}

// NOLINTBEGIN(readability-braces-around-statements)

/// Map a color name to its ANSI escape code.
/// Supports standard (30-37), bright (91-97), and 256-color (38;5;N) codes.
/// Returns empty string for "none", "default", or unknown names.
std::string color_name_to_ansi(const std::string& name) {
  if (name == "black") return "30";
  if (name == "red") return "31";
  if (name == "green") return "32";
  if (name == "yellow") return "33";
  if (name == "blue") return "34";
  if (name == "magenta") return "35";
  if (name == "cyan") return "36";
  if (name == "white") return "37";
  if (name == "bright-red") return "91";
  if (name == "bright-green") return "92";
  if (name == "bright-yellow") return "93";
  if (name == "bright-blue") return "94";
  if (name == "bright-magenta") return "95";
  if (name == "bright-cyan") return "96";
  if (name == "bright-white") return "97";
  if (name == "orange") return "38;5;208";
  if (name == "pink") return "38;5;213";
  if (name == "purple") return "38;5;129";
  if (name == "lime") return "38;5;118";
  if (name == "none" || name == "default") return "";
  return "";
}

/// Map an ANSI escape code back to a color name.
/// Inverse of color_name_to_ansi(). Returns "none" for unknown codes.
std::string ansi_to_name(const std::string& code) {
  if (code == "30") return "black";
  if (code == "31") return "red";
  if (code == "32") return "green";
  if (code == "33") return "yellow";
  if (code == "34") return "blue";
  if (code == "35") return "magenta";
  if (code == "36") return "cyan";
  if (code == "37") return "white";
  if (code == "91") return "bright-red";
  if (code == "92") return "bright-green";
  if (code == "93") return "bright-yellow";
  if (code == "94") return "bright-blue";
  if (code == "95") return "bright-magenta";
  if (code == "96") return "bright-cyan";
  if (code == "97") return "bright-white";
  if (code == "38;5;208") return "orange";
  if (code == "38;5;213") return "pink";
  if (code == "38;5;129") return "purple";
  if (code == "38;5;118") return "lime";
  return "none";
}

// NOLINTEND(readability-braces-around-statements)

// --- Shared subprocess helpers ---
// Used by multiple providers (kiro, opencode, tgpt) and subagent routing.

std::string shell_escape(const std::string& s) {
  std::string result = "'";
  for (char c : s) {
    if (c == '\'') {
      result += "'\\''";
    } else {
      result += c;
    }
  }
  result += "'";
  return result;
}

std::string strip_ansi(const std::string& s) {
  std::string result;
  bool in_escape = false;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\033') {
      in_escape = true;
      continue;
    }
    if (in_escape) {
      if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')) {
        in_escape = false;
      }
      continue;
    }
    result += s[i];
  }
  return result;
}

// --- Lazy hostname resolver with cache (ADR-089) ---
// Resolves IP addresses to hostnames on first access, caches the result.
// Uses getaddrinfo reverse lookup. Falls back to original string on failure.

#include <netdb.h>
#include <sys/socket.h>

#include <mutex>
#include <unordered_map>

std::string resolve_display_name(const std::string& host_port) {
  // Cache: avoid repeated DNS lookups for the same host
  static std::unordered_map<std::string, std::string> cache;
  static std::mutex mtx;

  std::lock_guard<std::mutex> lock(mtx);
  auto it = cache.find(host_port);
  if (it != cache.end()) return it->second;

  // Split host:port
  auto colon = host_port.rfind(':');
  if (colon == std::string::npos) {
    cache[host_port] = host_port;
    return host_port;
  }
  std::string ip = host_port.substr(0, colon);
  std::string port = host_port.substr(colon + 1);

  // Skip if already a hostname (contains letters)
  bool is_ip = true;
  for (char c : ip) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      is_ip = false;
      break;
    }
  }
  if (!is_ip) {
    cache[host_port] = host_port;
    return host_port;
  }

  // Reverse DNS lookup via getnameinfo (lazy — only on first display)
  struct addrinfo hints = {}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICHOST;
  if (getaddrinfo(ip.c_str(), nullptr, &hints, &res) != 0 || !res) {
    cache[host_port] = host_port;
    return host_port;
  }

  // getnameinfo does the actual PTR record / mDNS lookup
  char hostname[256] = {};
  int err = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, sizeof(hostname), nullptr, 0, 0);
  freeaddrinfo(res);

  std::string result;
  if (err == 0 && hostname[0] != '\0') {
    result = std::string(hostname) + ":" + port;
  } else {
    result = host_port;
  }
  cache[host_port] = result;
  return result;
}
