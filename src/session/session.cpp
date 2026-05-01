/**
 * @file session.cpp
 * @brief Session persistence — load/save conversation history as JSON (ADR-056).
 */

#include "session/session.h"

#include <fstream>
#include <iterator>
#include <string>

#include "json/json.h"

/// Load conversation history from a session JSON file.
/// Returns empty vector if file doesn't exist yet.
std::vector<Message> load_session(const std::string& path) {
  std::vector<Message> msgs;
  std::ifstream f(path);
  if (!f) {
    return msgs;  // new session — file will be created on save
  }
  std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  // Minimal parser: find each {"role":"...","content":"..."} object
  size_t pos = 0;
  while ((pos = json.find("\"role\"", pos)) != std::string::npos) {
    std::string role = json_extract_string(json.substr(pos - 1), "role");
    std::string content = json_extract_string(json.substr(pos - 1), "content");
    if (!role.empty()) {
      msgs.push_back({role, unescape_json(content)});
    }
    pos += 6;  // advance past "role"
  }
  return msgs;
}

/// Save conversation history to a session JSON file.
void save_session(const std::string& path, const std::vector<Message>& msgs) {
  std::ofstream f(path);
  f << "[\n";
  for (size_t i = 0; i < msgs.size(); i++) {
    f << "  {\"role\":\"" << msgs[i].role << "\",\"content\":\"" << escape_json(msgs[i].content) << "\"}";
    if (i + 1 < msgs.size()) {
      f << ",";
    }
    f << "\n";
  }
  f << "]\n";
}
