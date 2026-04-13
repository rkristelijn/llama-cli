/**
 * @file ollama.cpp
 * @brief Ollama API client using cpp-httplib.
 * @see docs/adr/adr-001-http-library.md
 * @see docs/adr/adr-004-configuration.md
 */

#include "ollama/ollama.h"

#include <httplib.h>

#include <chrono>
#include <iostream>
#include <string>

#include "json/json.h"
#include "logging/logger.h"
#include "tui/tui.h"

// Build an HTTP client for the configured Ollama instance
// Constructs URL from host:port config and sets read timeout
static httplib::Client make_client(const Config& cfg) {
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  httplib::Client cli(url);
  cli.set_read_timeout(cfg.timeout);
  return cli;
}

/// Escape quotes and newlines for JSON string (shortest representation)
/// @see https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
static std::string escape_json_string(const std::string& s) {
  std::string escaped;
  for (char c : s) {
    switch (c) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) <= 0x1f) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          escaped += buf;
        } else {
          escaped += c;
        }
    }
  }
  return escaped;
}

/** One-shot prompt via /api/generate (no conversation history)
 * Returns the response text, or empty string on connection error */
std::string ollama_generate(const Config& cfg, const std::string& prompt) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string escaped_prompt = escape_json_string(prompt);
  std::string body = R"({"model": ")" + cfg.model + R"(", "prompt": ")" + escaped_prompt + R"(", "stream": false})";

  auto res = cli.Post("/api/generate", body, "application/json");
  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!res) {
    tui::error(std::cerr, tui::use_color(cfg.no_color),
               "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "generate", prompt, "", duration, 0, 0);
    return "";
  }

  std::string response_text = json_extract_string(res->body, "response");
  int prompt_tokens = json_extract_int(res->body, "prompt_eval_count");
  int completion_tokens = json_extract_int(res->body, "eval_count");
  LOG_EVENT("ollama", "generate", prompt, response_text, duration, prompt_tokens, completion_tokens);
  return response_text;
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

/** Conversation via /api/chat (with message history)
 * Returns the assistant's response text, or empty string on error */
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string body =
      R"({"model": ")" + cfg.model + R"(", "messages": )" + build_messages_json(messages) + R"(, "stream": false})";

  auto res = cli.Post("/api/chat", body, "application/json");
  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!res) {
    tui::error(std::cerr, tui::use_color(cfg.no_color),
               "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "chat", build_messages_json(messages), "", duration, 0, 0);
    return "";
  }

  std::string response_text = json_extract_string(res->body, "content");
  int prompt_tokens = json_extract_int(res->body, "prompt_eval_count");
  int completion_tokens = json_extract_int(res->body, "eval_count");
  LOG_EVENT("ollama", "chat", build_messages_json(messages), response_text, duration, prompt_tokens, completion_tokens);
  return response_text;
}
