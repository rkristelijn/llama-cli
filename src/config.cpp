// config.cpp — Configuration loading implementation
// Precedence: CLI args > env vars > defaults
// Supports --long=value, --long value, -x value, and positional args

#include "config.h"

#include <cstdlib>
#include <string>

// Read an environment variable into a string, return true if set
static bool env_get(const char* name, std::string& out) {
  const char* val = std::getenv(name);
  if (val) {
    out = val;
  }
  return val != nullptr;
}

// Load config from environment variables, overriding defaults
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
  if (env_get("OLLAMA_TIMEOUT", val)) {
    c.timeout = std::stoi(val);
  }
  if (env_get("OLLAMA_SYSTEM_PROMPT", val)) {
    c.system_prompt = val;
  }
  return c;
}

/// CLI option definition: maps long/short flags to a config string field
struct OptDef {
  const char* long_prefix;      ///< e.g. "--host="
  const char* short_flag;       ///< e.g. "-h"
  std::string Config::* field;  ///< target field (nullptr for int)
};

// Table of string CLI options (timeout handled separately as int)
static const OptDef opts[] = {
    {"--host=", "-h", &Config::host},
    {"--port=", "-p", &Config::port},
    {"--model=", "-m", &Config::model},
    {"--timeout=", "-t", nullptr},
};

// Try to match arg against a long option prefix, return value if matched
static std::string match_long(const std::string& arg, const char* prefix) {
  std::string p(prefix);
  if (arg.rfind(p, 0) == 0) {
    return arg.substr(p.size());
  }
  return "";
}

// Try to match arg against all option definitions
// Iterates the opts table, tries long form first, then short form
// Returns true if matched, sets the config field via pointer-to-member
static bool match_opts(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  for (const auto& opt : opts) {
    std::string val = match_long(arg, opt.long_prefix);
    if (val.empty() && arg == opt.short_flag && i + 1 < argc) {
      val = argv[++i];
    }
    if (val.empty()) {
      continue;
    }
    if (opt.field) {
      c.*opt.field = val;
    } else {
      c.timeout = std::stoi(val);
    }
    return true;
  }
  return false;
}

// Load config from CLI arguments, overriding base config
// Parses long (--key=value), short (-x value), and positional args
Config load_cli(int argc, const char* const argv[], const Config& base) {
  Config c = base;

  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (match_opts(arg, i, argc, argv, c)) {
      continue;
    }

    // Positional arg = prompt (first non-option argument)
    // Triggers sync mode per ADR-005
    if (arg[0] != '-' && c.prompt.empty()) {
      c.prompt = arg;
      c.mode = Mode::Sync;
    }
  }
  return c;
}

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]) {
  Config c = load_env();
  return load_cli(argc, argv, c);
}
