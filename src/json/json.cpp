/**
 * @file json.cpp
 * @brief Minimal JSON string extraction for Ollama API responses.
 * @see docs/adr/adr-001-http-library.md
 */

#include "json/json.h"

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

/** Extract a JSON object by key: "key":{...}
 * Needed for Ollama's chat response where the assistant reply is nested:
 *   {"message":{"role":"assistant","content":"hello"}}
 * Returns the full object including braces, e.g. {"role":"assistant",...}
 * Tracks brace nesting depth to find the matching closing brace. */
std::string json_extract_object(const std::string& json, const std::string& key) {
  // Look for "key":{ pattern
  std::string needle = "\"" + key + "\":{";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return "";
  }
  pos += needle.size() - 1;  // point at opening {
  // Walk forward, tracking { and } depth until we find the matching close
  int depth = 0;
  for (size_t i = pos; i < json.size(); i++) {
    if (json[i] == '{') {
      depth++;
    } else if (json[i] == '}') {
      depth--;
      if (depth == 0) {
        return json.substr(pos, i - pos + 1);
      }
    }
  }
  return "";
}

/** Extract a JSON integer value by key: "key":123
 * Skips whitespace after the colon, then reads consecutive digits.
 * Returns 0 if the key is not found (sufficient for token counts). */

int json_extract_int(const std::string& json, const std::string& key) {
  // Find "key": pattern, then parse the digits that follow
  std::string needle = "\"" + key + "\":";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return 0;
  }
  pos += needle.size();
  // Skip whitespace after colon
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
    pos++;
  }
  int result = 0;
  for (size_t i = pos; i < json.size(); i++) {
    if (json[i] < '0' || json[i] > '9') {
      break;
    }
    result = result * 10 + (json[i] - '0');
  }
  return result;
}
