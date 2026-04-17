// test_config.cpp — Unit tests for config loading
// Uses Given/When/Then style per ADR-008

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "config/config.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

// Helper to clean all OLLAMA_ env vars
static void clean_env() {
  unsetenv("OLLAMA_HOST");
  unsetenv("OLLAMA_PORT");
  unsetenv("OLLAMA_MODEL");
  unsetenv("OLLAMA_TIMEOUT");
  unsetenv("LLAMA_EXEC_TIMEOUT");
  unsetenv("LLAMA_MAX_OUTPUT");
}

#include <fstream>

/// Write a temporary .env file, clean up on destruction
struct TmpEnvFile {
  std::string path;
  TmpEnvFile(const std::string& p, const std::string& content) : path(p) {
    std::ofstream f(p);
    f << content;
  }
  ~TmpEnvFile() { std::remove(path.c_str()); }
};

SCENARIO ("config defaults") {
  GIVEN ("no configuration is provided") {
    Config c;
    THEN ("defaults are used") {
      CHECK (c.host == "localhost")
        ;
      CHECK (c.port == "11434")
        ;
      CHECK (c.model == "gemma4:e4b")
        ;
      CHECK (c.timeout == 120)
        ;
      CHECK (c.exec_timeout == 30)
        ;
      CHECK (c.max_output == 10000)
        ;
      CHECK (c.mode == Mode::Interactive)
        ;
      CHECK (c.prompt.empty())
        ;
    }
  }
}

SCENARIO ("config from environment variables") {
  GIVEN ("all OLLAMA_ env vars are set") {
    clean_env();
    setenv("OLLAMA_HOST", "192.168.1.10", 1);
    setenv("OLLAMA_PORT", "9999", 1);
    setenv("OLLAMA_MODEL", "gemma4:26b", 1);
    setenv("OLLAMA_TIMEOUT", "60", 1);

    WHEN ("config is loaded from env") {
      Config c = load_env();
      THEN ("env vars override defaults") {
        CHECK (c.host == "192.168.1.10")
          ;
        CHECK (c.port == "9999")
          ;
        CHECK (c.model == "gemma4:26b")
          ;
        CHECK (c.timeout == 60)
          ;
      }
    }
    clean_env();
  }

  GIVEN ("no OLLAMA_ env vars are set") {
    clean_env();
    WHEN ("config is loaded from env") {
      Config c = load_env();
      THEN ("defaults are kept") {
        CHECK (c.host == "localhost")
          ;
        CHECK (c.port == "11434")
          ;
      }
    }
  }

  GIVEN ("OLLAMA_HOST contains host:port") {
    clean_env();
    setenv("OLLAMA_HOST", "0.0.0.0:11434", 1);
    WHEN ("config is loaded from env") {
      Config c = load_env();
      THEN ("host and port are split") {
        CHECK (c.host == "0.0.0.0")
          ;
        CHECK (c.port == "11434")
          ;
      }
    }
    clean_env();
  }
}

SCENARIO ("config from CLI arguments") {
  GIVEN ("long flags are provided") {
    const char* argv[] = {"llama-cli", "--host=10.0.0.1", "--model=gemma4:26b", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(3, argv);
      THEN ("flags override defaults") {
        CHECK (c.host == "10.0.0.1")
          ;
        CHECK (c.model == "gemma4:26b")
          ;
        CHECK (c.port == "11434")
          ;
      }
    }
  }

  GIVEN ("short flags are provided") {
    const char* argv[] = {"llama-cli", "-h", "10.0.0.1", "-m", "gemma4:26b", "-p", "9999", "-t", "60", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(9, argv);
      THEN ("short flags override defaults") {
        CHECK (c.host == "10.0.0.1")
          ;
        CHECK (c.model == "gemma4:26b")
          ;
        CHECK (c.port == "9999")
          ;
        CHECK (c.timeout == 60)
          ;
      }
    }
  }

  GIVEN ("a positional argument is provided") {
    const char* argv[] = {"llama-cli", "explain this error", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(2, argv);
      THEN ("it becomes the prompt in sync mode") {
        CHECK (c.prompt == "explain this error")
          ;
        CHECK (c.mode == Mode::Sync)
          ;
      }
    }
  }

  GIVEN ("no arguments are provided") {
    const char* argv[] = {"llama-cli", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(1, argv);
      THEN ("interactive mode is selected") {
        CHECK (c.prompt.empty())
          ;
        CHECK (c.mode == Mode::Interactive)
          ;
      }
    }
  }

  GIVEN ("options and a positional prompt are combined") {
    const char* argv[] = {"llama-cli", "--model=gemma4:26b", "review this code", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(3, argv);
      THEN ("options and prompt are both parsed") {
        CHECK (c.model == "gemma4:26b")
          ;
        CHECK (c.prompt == "review this code")
          ;
        CHECK (c.mode == Mode::Sync)
          ;
      }
    }
  }

  GIVEN ("unknown flags are provided") {
    const char* argv[] = {"llama-cli", "--unknown=value", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(2, argv);
      THEN ("they are ignored") {
        CHECK (c.host == "localhost")
          ;
      }
    }
  }

  GIVEN ("exec-timeout and max-output flags are provided") {
    const char* argv[] = {"llama-cli", "--exec-timeout=10", "--max-output=5000", nullptr};
    WHEN ("config is loaded from CLI") {
      Config c = load_cli(3, argv);
      THEN ("they are parsed") {
        CHECK (c.exec_timeout == 10)
          ;
        CHECK (c.max_output == 5000)
          ;
      }
    }
  }

  GIVEN ("exec settings via env vars") {
    clean_env();
    setenv("LLAMA_EXEC_TIMEOUT", "15", 1);
    setenv("LLAMA_MAX_OUTPUT", "2000", 1);
    WHEN ("config is loaded from env") {
      Config c = load_env();
      THEN ("env vars are used") {
        CHECK (c.exec_timeout == 15)
          ;
        CHECK (c.max_output == 2000)
          ;
      }
    }
    clean_env();
  }
}

SCENARIO ("config precedence chain") {
  GIVEN ("env vars and CLI args both set host") {
    clean_env();
    setenv("OLLAMA_HOST", "192.168.1.10", 1);
    setenv("OLLAMA_MODEL", "gemma4:26b", 1);

    const char* argv[] = {"llama-cli", "--host=localhost", "--model=cli-model", nullptr};
    WHEN ("full config is loaded") {
      Config c = load_config(3, argv);
      THEN ("CLI wins over everything for host") {
        CHECK (c.host == "localhost")
          ;
      }
      THEN ("CLI wins over everything for model") {
        CHECK (c.model == "cli-model")
          ;
      }
    }
    clean_env();
  }
}

SCENARIO ("config from .env file") {
  GIVEN ("a .env file with KEY=VALUE pairs") {
    clean_env();
    TmpEnvFile env("/tmp/llama-test.env", "OLLAMA_HOST=10.0.0.5\nOLLAMA_MODEL=test-model\n");
    Config c;
    load_dotenv("/tmp/llama-test.env", c);
    THEN ("values from .env are used") {
      CHECK (c.host == "10.0.0.5")
        ;
      CHECK (c.model == "test-model")
        ;
    }
    clean_env();
  }

  GIVEN ("a .env file with comments and blank lines") {
    clean_env();
    TmpEnvFile env("/tmp/llama-test2.env", "# comment\n\nOLLAMA_PORT=9999\n");
    Config c;
    load_dotenv("/tmp/llama-test2.env", c);
    THEN ("comments are skipped and values are loaded") {
      CHECK (c.port == "9999")
        ;
    }
    clean_env();
  }

  GIVEN ("a .env file with quoted values") {
    clean_env();
    TmpEnvFile env("/tmp/llama-test3.env", "OLLAMA_HOST=\"my-host\"\nOLLAMA_MODEL='my-model'\n");
    Config c;
    load_dotenv("/tmp/llama-test3.env", c);
    THEN ("quotes are stripped") {
      CHECK (c.host == "my-host")
        ;
      CHECK (c.model == "my-model")
        ;
    }
    clean_env();
  }

  GIVEN (".env overrides env vars") {
    clean_env();
    setenv("OLLAMA_HOST", "env-host", 1);
    TmpEnvFile env("/tmp/llama-test4.env", "OLLAMA_HOST=dotenv-host\n");
    Config c = load_env();
    load_dotenv("/tmp/llama-test4.env", c);
    THEN (".env wins over env var") {
      CHECK (c.host == "dotenv-host")
        ;
    }
    clean_env();
  }

  GIVEN ("no .env file exists") {
    clean_env();
    Config c;
    load_dotenv("/tmp/nonexistent-llama.env", c);
    THEN ("defaults are used") {
      CHECK (c.host == "localhost")
        ;
    }
    clean_env();
  }

  GIVEN (".env with host:port") {
    clean_env();
    TmpEnvFile env("/tmp/llama-test5.env", "OLLAMA_HOST=myhost:9999\n");
    Config c;
    load_dotenv("/tmp/llama-test5.env", c);
    THEN ("host and port are split") {
      CHECK (c.host == "myhost")
        ;
      CHECK (c.port == "9999")
        ;
    }
    clean_env();
  }
}

SCENARIO ("print default env template") {
  GIVEN ("the user requests a default .env template") {
    WHEN ("print_default_env is called") {
      std::stringstream buffer;
      std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
      print_default_env();
      std::cout.rdbuf(old);
      std::string output = buffer.str();

      THEN ("it contains all expected variables commented out") {
        CHECK (output.find("# llama-cli configuration template") != std::string::npos)
          ;
        CHECK (output.find("# LLAMA_PROVIDER=ollama") != std::string::npos)
          ;
        CHECK (output.find("# OLLAMA_HOST=localhost") != std::string::npos)
          ;
        CHECK (output.find("# OLLAMA_PORT=11434") != std::string::npos)
          ;
        CHECK (output.find("# OLLAMA_MODEL=gemma4:e4b") != std::string::npos)
          ;
        CHECK (output.find("# OLLAMA_SYSTEM_PROMPT=") != std::string::npos)
          ;
        CHECK (output.find("> .env") != std::string::npos)
          ;
      }
    }
  }
}