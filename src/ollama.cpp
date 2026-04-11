/**
 * @file ollama.cpp
 * @brief Ollama API client using cpp-httplib.
 * @see docs/adr/adr-001-http-library.md
 * @see docs/adr/adr-004-configuration.md
 */

#include "ollama.h"

#include <httplib.h>

#include <iostream>
#include <string>

#include "json.h"

// Build an HTTP client for the configured Ollama instance
// Constructs URL from host:port config and sets read timeout
static httplib::Client make_client(const Config& cfg) {
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  httplib::Client cli(url);
  cli.set_read_timeout(cfg.timeout);
  return cli;
}

// One-shot prompt via /api/generate (no conversation history)
// Returns the response text, or empty string on connection error
std::string ollama_generate(const Config& cfg, const std::string& prompt) {
  auto cli = make_client(cfg);
  std::string body = R"({"model": ")" + cfg.model + R"(", "prompt": ")" + prompt + R"(", "stream": false})";

  auto res = cli.Post("/api/generate", body, "application/json");
  if (!res) {
    std::cerr << "Error: could not connect to Ollama at " << cfg.host << ":" << cfg.port << "\n";
    return "";
  }
  return json_extract_string(res->body, "response");
}

// Build JSON messages array for /api/chat
// Escapes quotes and newlines in message content
static std::string build_messages_json(const std::vector<Message>& messages) {
  std::string json = "[";
  for (size_t i = 0; i < messages.size(); i++) {
    if (i > 0) {
      json += ",";
    }
    // Escape quotes in content
    std::string escaped;
    for (char c : messages[i].content) {
      if (c == '"') {
        escaped += "\\\"";
      } else if (c == '\n') {
        escaped += "\\n";
      } else {
        escaped += c;
      }
    }
    json += R"({"role":")" + messages[i].role + R"(","content":")" + escaped + R"("})";
  }
  json += "]";
  return json;
}

// Conversation via /api/chat (with message history)
// Returns the assistant's response text, or empty string on error
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages) {
  auto cli = make_client(cfg);
  std::string body =
      R"({"model": ")" + cfg.model + R"(", "messages": )" + build_messages_json(messages) + R"(, "stream": false})";

  auto res = cli.Post("/api/chat", body, "application/json");
  if (!res) {
    std::cerr << "Error: could not connect to Ollama at " << cfg.host << ":" << cfg.port << "\n";
    return "";
  }
  return json_extract_string(res->body, "content");
}
