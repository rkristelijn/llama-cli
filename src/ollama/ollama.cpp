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
#include <thread>

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

/// Escape special characters for safe embedding in a JSON string value.
/// Handles quotes, backslashes, control characters, and non-printable bytes.
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

/** One-shot prompt via /api/generate (no conversation history).
 * Sends a single prompt to Ollama and returns the response text.
 * Checks for API errors (e.g. model not found) and shows them to the user.
 * When trace is enabled, logs the HTTP request and response details to stderr.
 * All calls are logged to ~/.llama-cli/events.jsonl regardless of trace. */
// todo: reduce complexity of ollama_generate
// NOLINTNEXTLINE(readability-function-size)
std::string ollama_generate(const Config& cfg, const std::string& prompt) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  std::string escaped_prompt = escape_json_string(prompt);
  // stream:false = wait for complete response (no chunked streaming yet)
  std::string body = R"({"model": ")" + escape_json_string(cfg.model) + R"(", "prompt": ")" + escaped_prompt + R"(", "stream": false})";

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] POST %s/api/generate model=%s\n", url.c_str(), cfg.model.c_str());
  }

  auto res = cli.Post("/api/generate", body, "application/json");
  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Connection failed — Ollama is probably not running
  if (!res) {
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "generate", prompt, "", duration, 0, 0);
    return "";
  }

  // API error — e.g. model not found, invalid request
  if (res->status < 200 || res->status >= 300) {
    std::string err = json_extract_string(res->body, "error");
    if (err.empty()) {
      err = "HTTP " + std::to_string(res->status);
    }
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Ollama error: " + err);
    LOG_EVENT("ollama", "generate", prompt, err, duration, 0, 0);
    return "";
  }

  std::string response_text = json_extract_string(res->body, "response");
  int prompt_tokens = json_extract_int(res->body, "prompt_eval_count");
  int completion_tokens = json_extract_int(res->body, "eval_count");

  // Trace: show HTTP status, timing, and token usage
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] %d %dms tokens=%d/%d\n", res->status, duration, prompt_tokens, completion_tokens);
  }

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

/** Conversation via /api/chat (with message history).
 * Sends the full conversation history to Ollama and returns the assistant's reply.
 * The Ollama chat response nests the reply inside a "message" JSON object,
 * so we first extract that object, then extract "content" from it.
 * Checks for API errors and shows them to the user.
 * When trace is enabled, logs request/response details to stderr. */
// todo: reduce complexity of ollama_chat
// NOLINTNEXTLINE(readability-function-size)
std::string ollama_chat(const Config& cfg, const std::vector<Message>& messages) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  // stream:false = wait for complete response (no chunked streaming yet)
  std::string body =
      R"({"model": ")" + escape_json_string(cfg.model) + R"(", "messages": )" + build_messages_json(messages) + R"(, "stream": false})";

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] POST %s/api/chat model=%s messages=%zu\n", url.c_str(), cfg.model.c_str(), messages.size());
  }

  auto res = cli.Post("/api/chat", body, "application/json");
  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Connection failed — Ollama is probably not running
  if (!res) {
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "chat", build_messages_json(messages), "", duration, 0, 0);
    return "";
  }

  // API error — e.g. model not found, invalid request
  if (res->status < 200 || res->status >= 300) {
    std::string err = json_extract_string(res->body, "error");
    if (err.empty()) {
      err = "HTTP " + std::to_string(res->status);
    }
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Ollama error: " + err);
    LOG_EVENT("ollama", "chat", build_messages_json(messages), err, duration, 0, 0);
    return "";
  }

  // Ollama nests the reply in {"message":{"role":"assistant","content":"..."}}
  std::string message_json = json_extract_object(res->body, "message");
  std::string response_text = json_extract_string(message_json, "content");
  int prompt_tokens = json_extract_int(res->body, "prompt_eval_count");
  int completion_tokens = json_extract_int(res->body, "eval_count");

  // Trace: show HTTP status, timing, token usage, and truncated response
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] %d %dms tokens=%d/%d response=%.80s\n", res->status, duration, prompt_tokens, completion_tokens,
                      response_text.c_str());
  }

  LOG_EVENT("ollama", "chat", build_messages_json(messages), response_text, duration, prompt_tokens, completion_tokens);
  return response_text;
}

/** Stream a conversation via /api/chat with chunked response.
 * Each chunk from Ollama is a JSON object with {"message":{"content":"token"}}.
 * The final chunk has "done":true and includes token counts.
 * Calls on_token for each token; return false from callback to abort. */
// NOLINTNEXTLINE(readability-function-size)
std::string ollama_chat_stream(const Config& cfg, const std::vector<Message>& messages, StreamCallback on_token) {
  auto start = std::chrono::high_resolution_clock::now();
  auto cli = make_client(cfg);
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  // stream:true (default) — Ollama sends newline-delimited JSON chunks
  std::string body = R"({"model": ")" + escape_json_string(cfg.model) + R"(", "messages": )" + build_messages_json(messages) + "}";

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] POST %s/api/chat (stream) model=%s messages=%zu\n", url.c_str(), cfg.model.c_str(), messages.size());
  }

  std::string full_response;
  std::string buffer;  // partial line buffer for chunked reads
  int prompt_tokens = 0;
  int completion_tokens = 0;
  bool aborted = false;

  // Build request with content_receiver for streaming
  httplib::Request req;
  req.method = "POST";
  req.path = "/api/chat";
  req.set_header("Content-Type", "application/json");
  req.body = body;
  req.content_receiver = [&](const char* data, size_t len, uint64_t, uint64_t) {
    buffer.append(data, len);
    // Process complete lines (Ollama sends one JSON object per line)
    size_t pos = 0;
    while ((pos = buffer.find('\n')) != std::string::npos) {
      std::string line = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);
      if (line.empty()) {
        continue;
      }
      // Extract token from {"message":{"content":"..."}}
      std::string msg = json_extract_object(line, "message");
      std::string token = json_extract_string(msg, "content");
      if (!token.empty()) {
        full_response += token;
        if (on_token && !on_token(token)) {
          aborted = true;
          return false;  // abort stream
        }
      }
      // Final chunk has token counts
      if (line.find("\"done\":true") != std::string::npos) {
        prompt_tokens = json_extract_int(line, "prompt_eval_count");
        completion_tokens = json_extract_int(line, "eval_count");
      }
    }
    return true;
  };

  auto res = cli.send(req);

  auto end = std::chrono::high_resolution_clock::now();
  int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Retry once on connection failure — model switch can cause a brief unavailability
  if (!res && !aborted) {
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Connection failed, retrying in 2s (model may be loading)...");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    start = std::chrono::high_resolution_clock::now();
    res = cli.send(req);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  }

  if (!res && !aborted) {
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Error: could not connect to Ollama at " + cfg.host + ":" + cfg.port);
    LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), "", duration, 0, 0);
    return "";
  }

  if (res && (res->status < 200 || res->status >= 300)) {
    std::string err = json_extract_string(res->body, "error");
    if (err.empty()) {
      err = "HTTP " + std::to_string(res->status);
    }
    tui::error(std::cerr, tui::use_color(cfg.no_color), "Ollama error: " + err);
    LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), err, duration, 0, 0);
    return "";
  }

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] stream %dms tokens=%d/%d response=%.80s\n", duration, prompt_tokens, completion_tokens,
                      full_response.c_str());
  }

  LOG_EVENT("ollama", "chat_stream", build_messages_json(messages), full_response, duration, prompt_tokens, completion_tokens);
  return full_response;
}

/** Fetch list of available models from Ollama /api/tags endpoint.
 * Parses the JSON response and extracts model names.
 * Returns empty vector on connection error or invalid response. */
std::vector<std::string> get_available_models(const Config& cfg) {
  std::vector<std::string> models;
  auto cli = make_client(cfg);

  // GET /api/tags returns {"models":[{"name":"model1"},{"name":"model2"},...]}
  auto res = cli.Get("/api/tags");
  if (!res) {
    return models;  // Connection failed
  }

  // Parse models array from response
  // Extract the "models" array first, then find names within it
  std::string body = res->body;
  auto models_start = body.find("\"models\":");
  if (models_start == std::string::npos) {
    return models;
  }
  std::string models_section = body.substr(models_start);
  size_t pos = 0;
  while ((pos = models_section.find("\"name\":\"", pos)) != std::string::npos) {
    pos += 8;  // Skip past "name":"
    size_t end = models_section.find("\"", pos);
    if (end != std::string::npos) {
      models.push_back(models_section.substr(pos, end - pos));
      pos = end;
    }
  }

  return models;
}
