/// @file pii.cpp
/// @brief PII masking utility — replaces sensitive data in output (ADR-115)
#include "pii.h"

#include <cstdlib>
#include <regex>
#include <string>

// ADR-115: PII masking patterns
// - IPv4 addresses → *.*.*.*
// - IPv6 addresses → *:*::*
// - Machine hostname → <host>
// - Home path (/home/user) → /home/<user>
// - Email addresses → <email>
// - API keys (sk-*, AKIA*) → <key>

std::string mask_pii(const std::string& text) {
  std::string result = text;

  // IPv4: digits.digits.digits.digits
  static const std::regex ipv4(R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)");
  result = std::regex_replace(result, ipv4, "*.*.*.*");

  // IPv6: at least two hex groups with colons
  static const std::regex ipv6(R"(\b[0-9a-fA-F:]{6,39}\b)");
  result = std::regex_replace(result, ipv6, "*:*::*");

  // API keys: sk-... (OpenAI), AKIA... (AWS)
  static const std::regex api_key(R"(\b(sk-[a-zA-Z0-9]{20,}|AKIA[A-Z0-9]{16,})\b)");
  result = std::regex_replace(result, api_key, "<key>");

  // Email addresses
  static const std::regex email(R"(\b[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}\b)");
  result = std::regex_replace(result, email, "<email>");

  // Dynamic: hostname from $HOSTNAME
  const char* hostname = std::getenv("HOSTNAME");
  if (hostname && hostname[0] != '\0') {
    std::string h(hostname);
    size_t pos = 0;
    while ((pos = result.find(h, pos)) != std::string::npos) {
      result.replace(pos, h.size(), "<host>");
      pos += 6;
    }
  }

  // Dynamic: home path from $HOME and $USER
  const char* home = std::getenv("HOME");
  const char* user = std::getenv("USER");
  if (home && home[0] != '\0') {
    std::string h(home);
    size_t pos = 0;
    while ((pos = result.find(h, pos)) != std::string::npos) {
      std::string replacement = "/home/<user>";
      result.replace(pos, h.size(), replacement);
      pos += replacement.size();
    }
  } else if (user && user[0] != '\0') {
    // Fallback: replace /home/<actual-user>
    std::string pattern = std::string("/home/") + user;
    size_t pos = 0;
    while ((pos = result.find(pattern, pos)) != std::string::npos) {
      result.replace(pos, pattern.size(), "/home/<user>");
      pos += 12;
    }
  }

  return result;
}
