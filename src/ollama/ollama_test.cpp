/**
 * @file ollama_test.cpp
 * @brief Unit tests for Ollama API client using httplib::Server mock.
 *
 * Spins up a local HTTP server that mimics Ollama's API endpoints,
 * then exercises ollama_generate, ollama_chat, ollama_chat_stream,
 * and get_available_models against it.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "ollama/ollama.h"

#include <doctest/doctest.h>
#include <httplib.h>

#include <string>
#include <thread>
#include <vector>

/// RAII wrapper: starts httplib::Server on a free port, stops on destruction
struct MockServer {
  httplib::Server svr;
  std::thread thr;
  int port = 0;

  /// Start listening — call setup handlers before this
  void start() {
    port = svr.bind_to_any_port("127.0.0.1");
    thr = std::thread([this]() { svr.listen_after_bind(); });
    while (!svr.is_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  /// Stop server and join thread
  ~MockServer() {
    svr.stop();
    if (thr.joinable()) {
      thr.join();
    }
  }
};

/// Build a Config pointing at the mock server
static Config mock_cfg(int port) {
  Config c;
  c.host = "127.0.0.1";
  c.port = std::to_string(port);
  c.model = "test-model";
  c.timeout = 5;
  c.no_color = true;
  c.trace = false;
  c.system_prompt = "";
  return c;
}

/// Config pointing at nothing — for connection failure tests
static Config dead_cfg() {
  Config c;
  c.host = "127.0.0.1";
  c.port = "19999";
  c.model = "test";
  c.timeout = 1;
  c.no_color = true;
  c.trace = false;
  c.system_prompt = "";
  return c;
}

// --- ollama_generate ---

TEST_CASE ("ollama: generate returns response text") {
  MockServer m;
  m.svr.Post("/api/generate", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"response":"hello world","prompt_eval_count":5,"eval_count":2})", "application/json");
  });
  m.start();

  CHECK (ollama_generate(mock_cfg(m.port), "hi") == "hello world")
    ;
}

TEST_CASE ("ollama: generate handles connection failure") {
  CHECK (ollama_generate(dead_cfg(), "hi").empty())
    ;
}

TEST_CASE ("ollama: generate handles HTTP error with message") {
  MockServer m;
  m.svr.Post("/api/generate", [](const httplib::Request&, httplib::Response& res) {
    res.status = 404;
    res.set_content(R"({"error":"model not found"})", "application/json");
  });
  m.start();

  CHECK (ollama_generate(mock_cfg(m.port), "hi").empty())
    ;
}

TEST_CASE ("ollama: generate handles HTTP error without error field") {
  MockServer m;
  m.svr.Post("/api/generate", [](const httplib::Request&, httplib::Response& res) {
    res.status = 500;
    res.set_content("{}", "application/json");
  });
  m.start();

  CHECK (ollama_generate(mock_cfg(m.port), "hi").empty())
    ;
}

// --- ollama_chat ---

TEST_CASE ("ollama: chat returns assistant content") {
  MockServer m;
  m.svr.Post("/api/chat", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"message":{"role":"assistant","content":"I am helpful"},"prompt_eval_count":10,"eval_count":3})",
                    "application/json");
  });
  m.start();

  std::vector<Message> msgs = {{"user", "help me"}};
  CHECK (ollama_chat(mock_cfg(m.port), msgs) == "I am helpful")
    ;
}

TEST_CASE ("ollama: chat handles connection failure") {
  std::vector<Message> msgs = {{"user", "hi"}};
  CHECK (ollama_chat(dead_cfg(), msgs).empty())
    ;
}

TEST_CASE ("ollama: chat handles HTTP error") {
  MockServer m;
  m.svr.Post("/api/chat", [](const httplib::Request&, httplib::Response& res) {
    res.status = 400;
    res.set_content(R"({"error":"bad request"})", "application/json");
  });
  m.start();

  std::vector<Message> msgs = {{"user", "hi"}};
  CHECK (ollama_chat(mock_cfg(m.port), msgs).empty())
    ;
}

// --- ollama_chat_stream ---

TEST_CASE ("ollama: chat_stream collects tokens") {
  MockServer m;
  m.svr.Post("/api/chat", [](const httplib::Request&, httplib::Response& res) {
    std::string body;
    body += R"({"message":{"role":"assistant","content":"hello "},"done":false})"
            "\n";
    body += R"({"message":{"role":"assistant","content":"world"},"done":false})"
            "\n";
    body += R"({"message":{"role":"assistant","content":""},"done":true,"prompt_eval_count":5,"eval_count":2})"
            "\n";
    res.set_content(body, "application/json");
  });
  m.start();

  std::vector<std::string> tokens;
  std::vector<Message> msgs = {{"user", "hi"}};
  std::string result = ollama_chat_stream(mock_cfg(m.port), msgs, [&](const std::string& tok) {
    tokens.push_back(tok);
    return true;
  });
  CHECK (result == "hello world")
    ;
  CHECK (tokens.size() == 2)
    ;
}

TEST_CASE ("ollama: chat_stream handles connection failure") {
  std::vector<Message> msgs = {{"user", "hi"}};
  // Note: this test takes ~3s due to the 2s retry in production code
  CHECK (ollama_chat_stream(dead_cfg(), msgs, [](const std::string&) { return true; }).empty())
    ;
}

TEST_CASE ("ollama: chat_stream handles HTTP error") {
  MockServer m;
  m.svr.Post("/api/chat", [](const httplib::Request&, httplib::Response& res) {
    res.status = 500;
    res.set_content(R"({"error":"internal error"})", "application/json");
  });
  m.start();

  std::vector<Message> msgs = {{"user", "hi"}};
  CHECK (ollama_chat_stream(mock_cfg(m.port), msgs, [](const std::string&) { return true; }).empty())
    ;
}

// --- get_available_models ---

TEST_CASE ("ollama: get_available_models parses model list") {
  MockServer m;
  m.svr.Get("/api/tags", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"models":[{"name":"gemma4:e4b"},{"name":"qwen2.5:14b"}]})", "application/json");
  });
  m.start();

  auto models = get_available_models(mock_cfg(m.port));
  CHECK (models.size() == 2)
    ;
  CHECK (models[0] == "gemma4:e4b")
    ;
  CHECK (models[1] == "qwen2.5:14b")
    ;
}

TEST_CASE ("ollama: get_available_models returns empty on connection failure") {
  CHECK (get_available_models(dead_cfg()).empty())
    ;
}

TEST_CASE ("ollama: get_available_models returns empty on bad response") {
  MockServer m;
  m.svr.Get("/api/tags", [](const httplib::Request&, httplib::Response& res) { res.set_content("{}", "application/json"); });
  m.start();

  CHECK (get_available_models(mock_cfg(m.port)).empty())
    ;
}

// --- trace mode ---

TEST_CASE ("ollama: generate with trace logs to stderr") {
  MockServer m;
  m.svr.Post("/api/generate", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"response":"traced","prompt_eval_count":1,"eval_count":1})", "application/json");
  });
  m.start();

  auto cfg = mock_cfg(m.port);
  cfg.trace = true;
  Config::instance().trace = true;

  CHECK (ollama_generate(cfg, "test") == "traced")
    ;
  Config::instance().trace = false;
}

TEST_CASE ("ollama: chat with trace logs to stderr") {
  MockServer m;
  m.svr.Post("/api/chat", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"message":{"role":"assistant","content":"traced"},"prompt_eval_count":1,"eval_count":1})", "application/json");
  });
  m.start();

  auto cfg = mock_cfg(m.port);
  cfg.trace = true;
  Config::instance().trace = true;

  std::vector<Message> msgs = {{"user", "test"}};
  CHECK (ollama_chat(cfg, msgs) == "traced")
    ;
  Config::instance().trace = false;
}
