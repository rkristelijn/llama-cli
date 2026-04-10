// ollama.cpp — Ollama API client implementation
// Uses cpp-httplib for HTTP and json.h for response parsing.

#include "ollama.h"

#include <httplib.h>

#include <iostream>
#include <string>

#include "json.h"

std::string ollama_generate(const Config& cfg, const std::string& prompt) {
  std::string url = "http://" + cfg.host + ":" + cfg.port;
  httplib::Client cli(url);
  cli.set_read_timeout(cfg.timeout);

  // Build JSON request body
  std::string body = R"({"model": ")" + cfg.model + R"(", "prompt": ")" + prompt + R"(", "stream": false})";

  auto res = cli.Post("/api/generate", body, "application/json");
  if (!res) {
    std::cerr << "Error: could not connect to Ollama at " << url << "\n";
    return "";
  }
  return json_extract_string(res->body, "response");
}
