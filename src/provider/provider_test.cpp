/**
 * @file provider_test.cpp
 * @brief Unit tests for provider abstraction layer (ADR-020).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <cstdlib>
#include <stdexcept>

#include "provider/provider_factory.h"

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
