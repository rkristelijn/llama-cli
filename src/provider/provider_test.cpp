/**
 * @file provider_test.cpp
 * @brief Unit tests for provider abstraction layer (ADR-020).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <httplib.h>

#include <cstdlib>
#include <stdexcept>
#include <thread>

#include "provider/multi_host_provider.h"
#include "provider/provider_factory.h"
#include "provider/registry.h"

// NOLINTNEXTLINE(readability-function-size)
SCENARIO ("provider factory: mock provider echoes prompts") {
  GIVEN ("a config with provider set to mock") {
    Config cfg;
    cfg.provider = "mock";
    // Ensure no env var interferes
    unsetenv("LLAMA_CLI_MOCK_RESPONSE");

    WHEN ("creating a provider") {
      auto provider = create_provider(cfg);

      THEN ("it reports correct name and host") {
        CHECK (provider->name() == "mock")
          ;
        CHECK (provider->host() == "localhost:0")
          ;
      }

      THEN ("chat echoes the last message content") {
        std::vector<Message> msgs = {{"user", "hello world"}};
        CHECK (provider->chat(msgs) == "mock response: hello world")
          ;
      }

      THEN ("chat with empty messages returns empty echo") {
        CHECK (provider->chat({}) == "mock response: ")
          ;
      }

      THEN ("chat_stream calls on_token and returns full response") {
        std::vector<Message> msgs = {{"user", "test prompt"}};
        std::string streamed;
        auto result = provider->chat_stream(msgs, [&](const std::string& token) {
          streamed += token;
          return true;
        });
        CHECK (result == "mock response: test prompt")
          ;
        CHECK (streamed == "mock response: test prompt")
          ;
      }

      THEN ("list_models returns mock model") {
        auto models = provider->list_models();
        CHECK (models.size() == 1)
          ;
        CHECK (models[0] == "mock-model")
          ;
      }

      THEN ("is_model_running always returns true") {
        CHECK (provider->is_model_running("anything"))
          ;
      }
    }
  }
}

SCENARIO ("provider factory: mock provider uses env var response") {
  GIVEN ("LLAMA_CLI_MOCK_RESPONSE is set") {
    setenv("LLAMA_CLI_MOCK_RESPONSE", "custom response", 1);
    Config cfg;
    cfg.provider = "mock";

    WHEN ("chatting via mock provider") {
      auto provider = create_provider(cfg);
      std::vector<Message> msgs = {{"user", "ignored"}};

      THEN ("response comes from env var, not echo") {
        CHECK (provider->chat(msgs) == "custom response")
          ;
      }
    }

    // Cleanup
    unsetenv("LLAMA_CLI_MOCK_RESPONSE");
  }
}

SCENARIO ("provider factory: ollama provider has correct metadata") {
  GIVEN ("a config with ollama provider and custom host") {
    Config cfg;
    cfg.provider = "ollama";
    cfg.host = "apsnlmac4050";
    cfg.port = "11434";

    WHEN ("creating the provider") {
      auto provider = create_provider(cfg);

      THEN ("name is ollama") {
        CHECK (provider->name() == "ollama")
          ;
      }

      THEN ("host reflects config") {
        CHECK (provider->host() == "apsnlmac4050:11434")
          ;
      }
    }
  }
}

SCENARIO ("provider factory: empty provider defaults to ollama") {
  GIVEN ("a config with empty provider string") {
    Config cfg;
    cfg.provider = "";
    cfg.host = "localhost";
    cfg.port = "11434";

    WHEN ("creating the provider") {
      auto provider = create_provider(cfg);

      THEN ("it creates an ollama provider") {
        CHECK (provider->name() == "ollama")
          ;
      }
    }
  }
}

SCENARIO ("provider factory: unknown provider throws") {
  GIVEN ("a config with an unsupported provider name") {
    Config cfg;
    cfg.provider = "nonexistent";

    WHEN ("creating the provider") {
      THEN ("it throws runtime_error") {
        CHECK_THROWS_AS(create_provider(cfg), std::runtime_error);
      }
    }
  }
}

SCENARIO ("provider factory: tgpt provider has correct metadata") {
  GIVEN ("a config with tgpt provider") {
    Config cfg;
    cfg.provider = "tgpt";

    WHEN ("creating the provider") {
      auto provider = create_provider(cfg);

      THEN ("name is tgpt") {
        CHECK (provider->name() == "tgpt")
          ;
      }

      THEN ("host is local-cli") {
        CHECK (provider->host() == "local-cli")
          ;
      }

      THEN ("list_models returns tgpt-default") {
        auto models = provider->list_models();
        CHECK (models.size() == 1)
          ;
        CHECK (models[0] == "tgpt-default")
          ;
      }
    }
  }
}

SCENARIO ("provider factory: gemini provider has correct metadata") {
  GIVEN ("a config with gemini provider") {
    Config cfg;
    cfg.provider = "gemini";

    WHEN ("creating the provider") {
      auto provider = create_provider(cfg);

      THEN ("name is gemini") {
        CHECK (provider->name() == "gemini")
          ;
      }

      THEN ("host is local-cli") {
        CHECK (provider->host() == "local-cli")
          ;
      }

      THEN ("list_models returns gemini-cli") {
        auto models = provider->list_models();
        CHECK (models.size() == 1)
          ;
        CHECK (models[0] == "gemini-cli")
          ;
      }
    }
  }
}

// --- MultiHostProvider tests (ADR-078) ---

/// RAII mock server for Ollama API
struct MockOllama {
  httplib::Server svr;
  std::thread thr;
  int port = 0;

  void start() {
    port = svr.bind_to_any_port("127.0.0.1");
    thr = std::thread([this]() { svr.listen_after_bind(); });
    while (!svr.is_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~MockOllama() {
    svr.stop();
    if (thr.joinable()) {
      thr.join();
    }
  }
};

SCENARIO ("multi_host: model is passed to per-host providers for chat") {
  GIVEN ("a mock Ollama server that validates the model field") {
    MockOllama m;
    // Return model list for probing
    m.svr.Get("/api/tags", [](const httplib::Request&, httplib::Response& res) {
      res.set_content(R"({"models":[{"name":"test-model"}]})", "application/json");
    });
    m.svr.Get("/api/ps", [](const httplib::Request&, httplib::Response& res) { res.set_content(R"({"models":[]})", "application/json"); });
    // Chat endpoint: 404 if model is empty, 200 if correct
    m.svr.Post("/api/chat", [](const httplib::Request& req, httplib::Response& res) {
      if (req.body.find("\"model\": \"test-model\"") == std::string::npos) {
        res.status = 404;
        res.set_content(R"({"error":"model not found"})", "application/json");
        return;
      }
      // Return a simple streaming response
      res.set_content(
          "{\"message\":{\"content\":\"hi\"},\"done\":true,"
          "\"prompt_eval_count\":1,\"eval_count\":1}\n",
          "application/json");
    });
    m.start();

    WHEN ("creating a MultiHostProvider with a specific model") {
      std::vector<std::string> hosts = {"127.0.0.1:" + std::to_string(m.port)};
      MultiHostProvider provider(hosts, "test-model");

      THEN ("chat_stream sends the model and gets a response") {
        std::vector<Message> msgs = {{"user", "hello"}};
        std::string result = provider.chat_stream(msgs, nullptr);
        // Should NOT be empty — empty means 404 (model not passed)
        CHECK_FALSE (result.empty())
          ;
      }
    }
  }
}

SCENARIO ("model selection: provider and host are set correctly") {
  GIVEN ("a registry with models from different providers") {
    ModelRegistry reg;
    reg.models = {{"gemma4:e4b", "ollama", "localhost:11434", "", 42.0, 4.0, 0, CostTier::Free, {Capability::General}, true},
                  {"amazon-q", "amazon-q", "cloud", "", 0, 0, 0, CostTier::Free, {Capability::Code}, true},
                  {"gemini-cli", "gemini", "cloud", "", 0, 0, 0, CostTier::Free, {Capability::General}, true}};

    WHEN ("selecting an ollama model") {
      Config::instance().model = "gemma4:e4b";
      Config::instance().provider = "ollama";
      Config::instance().host = "localhost";
      Config::instance().port = "11434";

      THEN ("config has correct values") {
        CHECK (Config::instance().model == "gemma4:e4b")
          ;
        CHECK (Config::instance().provider == "ollama")
          ;
        CHECK (Config::instance().host == "localhost")
          ;
        CHECK (Config::instance().port == "11434")
          ;
      }
    }

    WHEN ("selecting amazon-q model") {
      Config::instance().model = "amazon-q";
      Config::instance().provider = "amazon-q";

      THEN ("config has correct provider") {
        CHECK (Config::instance().model == "amazon-q")
          ;
        CHECK (Config::instance().provider == "amazon-q")
          ;
      }
    }

    WHEN ("selecting gemini model") {
      Config::instance().model = "gemini-cli";
      Config::instance().provider = "gemini";

      THEN ("config has correct provider") {
        CHECK (Config::instance().model == "gemini-cli")
          ;
        CHECK (Config::instance().provider == "gemini")
          ;
      }
    }
  }
}
