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
#include "trace/trace.h"
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
    json += R"({"role":")" + messages[i].role + R"(","content":")" + escape_json_string(messages[i].content) + R"("})";
  }
  json += "]";
  return json;
}

/** Conversation via /api/chat (with message history)
 * Returns the assistant's response text, or empty string on error */
// NOLINTNEXTLINE(readability-function-size) — trace logging adds lines but not complexity
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages, Trace* trace) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string url = "http://" + cfg.host + ":" + cfg.port + "/api/chat";
  std::string body =
      R"({"model": ")" + cfg.model + R"(", "messages": )" + build_messages_json(messages) + R"(, "stream": false})";

  if (trace) {
    trace->log("[TRACE] http POST %s model=%s messages=%zu\n", url.c_str(), cfg.model.c_str(), messages.size());
  }

  auto res = cli.Post("/api/chat", body, "application/json");
  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!res) {
    if (trace) {
      trace->log("[TRACE] http error: no response from %s (connection refused or timeout after %dms)\n", url.c_str(),
                 duration);
    }
    tui::error(std::cerr, tui::use_color(cfg.no_color),
               "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "chat", build_messages_json(messages), "", duration, 0, 0);
    return "";
  }

  if (trace) {
    trace->log("[TRACE] http status=%d duration=%dms\n", res->status, duration);
  }

  std::string message_json_string = json_extract_string(res->body, "message");
  std::string response_text = json_extract_string(message_json_string, "content");
  int prompt_tokens = json_extract_int(res->body, "prompt_eval_count");
  int completion_tokens = json_extract_int(res->body, "eval_count");

  if (trace) {
    trace->log("[TRACE] tokens prompt=%d completion=%d response_len=%zu\n", prompt_tokens, completion_tokens,
               response_text.size());
  }

  LOG_EVENT("ollama", "chat", build_messages_json(messages), response_text, duration, prompt_tokens, completion_tokens);
  return response_text;
}
