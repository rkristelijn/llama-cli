/**
 * @file json.cpp
 * @brief Minimal JSON string extraction for Ollama API responses.
 * @see docs/adr/adr-001-http-library.md
 */

#include "json/json.h"

#include <unordered_map>

/** Simple escape map: JSON escape char -> decoded char. */
static const std::unordered_map<char, char> escape_map = {
    {'n', '\n'}, {'t', '\t'}, {'r', '\r'}, {'b', '\b'}, {'f', '\f'}, {'"', '"'}, {'\\', '\\'}, {'/', '/'},
};

/** Decode \uXXXX escape (ASCII range only).
 * Returns extra chars consumed, or 0 on failure. */
static int decode_unicode_escape(const std::string& json, size_t i, std::string& out) {
  if (i + 5 >= json.size()) {
    return 0;
  }
  unsigned long cp = std::stoul(json.substr(i + 2, 4), nullptr, 16);
  if (cp < 128) {
    out += static_cast<char>(cp);
  }
  return 5;
}

/** Decode a single JSON escape sequence starting at backslash position.
 * Returns number of extra chars consumed (beyond the backslash). */
static int decode_escape(const std::string& json, size_t i, std::string& out) {
  if (i + 1 >= json.size()) {
    return 0;
  }
  char next = json[i + 1];
  auto it = escape_map.find(next);
  if (it != escape_map.end()) {
    out += it->second;
    return 1;
  }
  if (next == 'u') {
    return decode_unicode_escape(json, i, out);
  }
  return 0;
}

/** Check if character at position is an unescaped quote (end of JSON string).
 * Counts consecutive backslashes before the quote — an even count means
 * the quote is real, an odd count means it is escaped. */
static bool is_end_quote(const std::string& json, size_t i) {
  if (json[i] != '"') {
    return false;
  }
  // Count consecutive backslashes before this quote
  int backslashes = 0;
  while (i >= static_cast<size_t>(backslashes + 1) && json[i - 1 - backslashes] == '\\') {
    backslashes++;
  }
  // Even number of backslashes = real quote, odd = escaped quote
  return (backslashes % 2) == 0;
}

/** Skip whitespace characters (space, tab, newline, carriage return). */
static size_t skip_ws(const std::string& json, size_t pos) {
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
    pos++;
  }
  return pos;
}

/** Find position after "key": pattern, skipping whitespace after colon. */
static size_t find_key_value(const std::string& json, const std::string& key) {
  std::string needle = "\"" + key + "\":";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return std::string::npos;
  }
  return skip_ws(json, pos + needle.size());
}

/** Walk forward from an opening brace, tracking depth, return matching object. */
static std::string extract_braced(const std::string& json, size_t pos) {
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (size_t i = pos; i < json.size(); i++) {
    char c = json[i];
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
    } else if (c == '{') {
      depth++;
    } else if (c == '}') {
      depth--;
      if (depth == 0) {
        return json.substr(pos, i - pos + 1);
      }
    }
  }
  return "";
}

/** Extract a JSON string value by key: "key":"value" or "key": "value"
 * Walks the string char-by-char, decoding escape sequences */
std::string json_extract_string(const std::string& json, const std::string& key) {
  size_t pos = find_key_value(json, key);
  if (pos == std::string::npos || pos >= json.size() || json[pos] != '"') {
    return "";
  }
  pos++;  // skip opening quote
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
  size_t pos = find_key_value(json, key);
  if (pos == std::string::npos || pos >= json.size() || json[pos] != '{') {
    return "";
  }
  return extract_braced(json, pos);
}

/** Extract a JSON object starting at a given position.
 * pos must point to '{'. Used to walk arrays of objects. */
std::string json_extract_object_at(const std::string& json, size_t pos) {
  if (pos >= json.size() || json[pos] != '{') {
    return "";
  }
  return extract_braced(json, pos);
}

/** Extract a JSON integer value by key: "key":123
 * Skips whitespace after the colon, then reads consecutive digits.
 * Returns 0 if the key is not found (sufficient for token counts). */
int json_extract_int(const std::string& json, const std::string& key) {
  size_t pos = find_key_value(json, key);
  if (pos == std::string::npos) {
    return 0;
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

/// Escape a string for JSON output (quotes, backslashes, control chars)
std::string escape_json(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
    }
  }
  return out;
}

/// Unescape JSON string escape sequences (\\n, \\t, \\", \\\\, etc.)
std::string unescape_json(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      switch (s[++i]) {
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case 'n':
          out += '\n';
          break;
        case 'r':
          out += '\r';
          break;
        case 't':
          out += '\t';
          break;
        default:
          out += '\\';
          out += s[i];
      }
    } else {
      out += s[i];
    }
  }
  return out;
}