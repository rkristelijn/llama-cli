// json.cpp — Minimal JSON string extraction
// Handles escaped characters (\n, \", \\) in JSON string values.

#include "json.h"

std::string json_extract_string(const std::string& json, const std::string& key) {
  // Find "key":"
  std::string needle = "\"" + key + "\":\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) return "";

  pos += needle.size();
  std::string result;
  for (size_t i = pos; i < json.size(); i++) {
    // Unescaped quote = end of value
    if (json[i] == '"' && (i == 0 || json[i - 1] != '\\')) break;
    // Handle escape sequences
    if (json[i] == '\\' && i + 1 < json.size()) {
      char next = json[i + 1];
      if (next == 'n') {
        result += '\n';
        i++;
        continue;
      }
      if (next == '"') {
        result += '"';
        i++;
        continue;
      }
      if (next == '\\') {
        result += '\\';
        i++;
        continue;
      }
      // Handle unicode escapes: \uXXXX (common: \u003c = <, \u003e = >)
      if (next == 'u' && i + 5 < json.size()) {
        std::string hex = json.substr(i + 2, 4);
        unsigned long cp = std::stoul(hex, nullptr, 16);
        if (cp < 128) {
          result += static_cast<char>(cp);
        }
        i += 5;
        continue;
      }
    }
    result += json[i];
  }
  return result;
}
