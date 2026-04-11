/**
 * @file json.cpp
 * @brief Minimal JSON string extraction for Ollama API responses.
 * @see docs/adr/adr-001-http-library.md
 */

#include "json.h"

/** Decode a single JSON escape sequence starting at backslash position
 * Supports \n, \", \\, and \uXXXX (ASCII range only)
 * Returns number of extra chars consumed (beyond the backslash) */
static int decode_escape(const std::string& json, size_t i, std::string& out) {
  if (i + 1 >= json.size()) {
    return 0;
  }
  char next = json[i + 1];
  if (next == 'n') {
    out += '\n';
    return 1;
  }
  if (next == '"') {
    out += '"';
    return 1;
  }
  if (next == '\\') {
    out += '\\';
    return 1;
  }
  if (next == 'u' && i + 5 < json.size()) {
    unsigned long cp = std::stoul(json.substr(i + 2, 4), nullptr, 16);
    if (cp < 128) {
      out += static_cast<char>(cp);
    }
    return 5;
  }
  return 0;
}

/** Check if character at position is an unescaped quote (end of JSON string) */
static bool is_end_quote(const std::string& json, size_t i) {
  return json[i] == '"' && (i == 0 || json[i - 1] != '\\');
}

/** Extract a JSON string value by key: "key":"value"
 * Walks the string char-by-char, decoding escape sequences */
std::string json_extract_string(const std::string& json, const std::string& key) {
  std::string needle = "\"" + key + "\":\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return "";
  }

  pos += needle.size();
  std::string result;
  for (size_t i = pos; i < json.size(); i++) {
    if (is_end_quote(json, i)) {
      break;
    }
    if (json[i] == '\\') {
      int skip = decode_escape(json, i, result);
      if (skip > 0) {
        i += skip;
        continue;
      }
    }
    result += json[i];
  }
  return result;
}
