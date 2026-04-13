/**
 * @file config_test_helper.cpp
 * @brief Shared config implementation for tests (avoids linking full REPL_LIBS)
 */

#include <cstdlib>
#include <sstream>
#include <string>

#include "config/config.h"

// Minimal implementations needed for config tests
bool env_get(const char* name, std::string& out) {
  const char* val = std::getenv(name);
  if (!val) {
    return false;
  }
  out = val;
  return true;
}

Config load_env(const Config& defaults) {
  Config c = defaults;
  std::string val;
  if (env_get("OLLAMA_HOST", val)) {
    c.host = val;
  }
  if (env_get("OLLAMA_PORT", val)) {
    c.port = val;
  }
  if (env_get("OLLAMA_MODEL", val)) {
    c.model = val;
  }
  if (env_get("NO_COLOR", val)) {
    c.no_color = true;
  }
  return c;
}

std::string match_long(const std::string& arg, const char* prefix) {
  std::string p(prefix);
  if (arg.rfind(p, 0) == 0) {
    return arg.substr(p.size());
  }
  return "";
}

// clang-tidy:skip-complexity — TODO: refactor (see TODO.md)
Config load_cli(int argc, const char* const argv[], const Config& base) {
  Config c = base;
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);

    // --model=VALUE
    std::string model_val = match_long(arg, "--model=");
    if (!model_val.empty()) {
      c.model = model_val;
      continue;
    }

    // --files=FILE or --files FILE
    std::string files_val = match_long(arg, "--files=");
    if (!files_val.empty() || arg == "--files") {
      if (files_val.empty() && i + 1 < argc) {
        files_val = argv[++i];
      }
      if (!files_val.empty()) {
        std::istringstream iss(files_val);
        std::string path;
        while (iss >> path) {
          c.files.push_back(path);
        }
        c.mode = Mode::Sync;
      }
      continue;
    }

    // Positional arg = prompt
    if (arg[0] != '-' && c.prompt.empty()) {
      c.prompt = arg;
      c.mode = Mode::Sync;
    }
  }
  return c;
}

Config load_config(int argc, const char* const argv[]) {
  Config c = load_env();
  return load_cli(argc, argv, c);
}
