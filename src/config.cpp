/**
 * @file config.cpp
 * @brief Configuration loading: defaults → env vars → CLI args.
 * @see docs/adr/adr-004-configuration.md
 * @see docs/adr/adr-015-command-execution.md
 */

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
  if (env_get("LLAMA_EXEC_TIMEOUT", val)) {
    c.exec_timeout = std::stoi(val);
  }
  if (env_get("LLAMA_MAX_OUTPUT", val)) {
    c.max_output = std::stoi(val);
  }
  if (env_get("OLLAMA_SYSTEM_PROMPT", val)) {
    c.system_prompt = val;
  }
  return c;
}

// Pointer-to-member type for Config string fields
using ConfigField = std::string Config::*;

/**
 * CLI option definition: maps long/short flags to a config field.
 */
struct OptDef {
  /** Long option prefix, e.g. "--host=" */
  const char* long_prefix;
  /** Short option flag, e.g. "-h" */
  const char* short_flag;
  /** Pointer-to-member for the target Config string field (nullptr for int) */
  ConfigField field;
};

// Table of string CLI options
static const OptDef opts[] = {
    {"--host=", "-h", &Config::host},
    {"--port=", "-p", &Config::port},
    {"--model=", "-m", &Config::model},
};

/** CLI int option definition: maps long/short flag to a config int field. */
struct IntOptDef {
  /** Long option prefix, e.g. "--timeout=" */
  const char* long_prefix;
  /** Short option flag, e.g. "-t" (nullptr if none) */
  const char* short_flag;
  /** Pointer-to-member for the target Config int field */
  int Config::* field;
};

// Table of int CLI options
static const IntOptDef int_opts[] = {
    {"--timeout=", "-t", &Config::timeout},
    {"--exec-timeout=", nullptr, &Config::exec_timeout},
    {"--max-output=", nullptr, &Config::max_output},
};

// Try to match arg against a long option prefix, return value if matched
static std::string match_long(const std::string& arg, const char* prefix) {
  std::string p(prefix);
  if (arg.rfind(p, 0) == 0) {
    return arg.substr(p.size());
  }
  return "";
}

// Try to match arg against string option definitions
static bool match_string_opts(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  for (const auto& opt : opts) {
    std::string val = match_long(arg, opt.long_prefix);
    if (val.empty() && arg == opt.short_flag && i + 1 < argc) {
      val = argv[++i];
    }
    if (!val.empty()) {
      c.*opt.field = val;
      return true;
    }
  }
  return false;
}

// Try to match arg against int option definitions
static bool match_int_opts(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  for (const auto& opt : int_opts) {
    std::string val = match_long(arg, opt.long_prefix);
    if (val.empty() && opt.short_flag && arg == opt.short_flag && i + 1 < argc) {
      val = argv[++i];
    }
    if (!val.empty()) {
      c.*opt.field = std::stoi(val);
      return true;
    }
  }
  return false;
}

// Try to match arg against all option definitions
// Returns true if matched, sets the config field via pointer-to-member
static bool match_opts(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  return match_string_opts(arg, i, argc, argv, c) || match_int_opts(arg, i, argc, argv, c);
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
