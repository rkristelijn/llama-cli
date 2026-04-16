/**
 * @file config.cpp
 * @brief Configuration loading: defaults → env vars → CLI args.
 * @see docs/adr/adr-004-configuration.md
 * @see docs/adr/adr-015-command-execution.md
 */

#include "config/config.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/** Read an environment variable into a string, return true if set */
static bool env_get(const char* name, std::string& out) {
  const char* val = std::getenv(name);
  if (val) {
    out = val;
  }
  return val != nullptr;
}

/** Load KEY=VALUE pairs from a .env file into a Config struct.
 * Skips blank lines, comments (#), and lines without '='.
 * Overrides values already in the config (project-local wins over env). */
// todo: reduce complexity of load_dotenv
// NOLINTNEXTLINE(readability-function-size)
void load_dotenv(const std::string& path, Config& c) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return;
  }
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);
    // Strip optional surrounding quotes from value
    if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
      val = val.substr(1, val.size() - 2);
    }
    if (key == "OLLAMA_HOST") {
      // Support host:port format (e.g. OLLAMA_HOST=0.0.0.0:11434)
      // Split on last colon to separate host from port
      auto colon = val.rfind(':');
      if (colon != std::string::npos) {
        c.host = val.substr(0, colon);
        c.port = val.substr(colon + 1);
      } else {
        c.host = val;
      }
    } else if (key == "OLLAMA_PORT") {
      c.port = val;
    } else if (key == "OLLAMA_MODEL") {
      c.model = val;
    } else if (key == "OLLAMA_TIMEOUT") {
      c.timeout = std::stoi(val);
    } else if (key == "LLAMA_EXEC_TIMEOUT") {
      c.exec_timeout = std::stoi(val);
    } else if (key == "LLAMA_MAX_OUTPUT") {
      c.max_output = std::stoi(val);
    } else if (key == "OLLAMA_SYSTEM_PROMPT") {
      c.system_prompt = val;
    } else if (key == "LLAMA_PROVIDER") {
      c.provider = val;
    } else if (key == "NO_COLOR") {
      c.no_color = true;
    } else if (key == "TRACE") {
      // TRACE=true or TRACE=1 enables trace output (HTTP calls, timing)
      c.trace = (val == "true" || val == "1");
    }
  }
}

/** Validate the configuration, print errors and exit on failure */
static void validate_config(const Config& c) {
  bool ok = true;
  if (c.host.empty()) {
    std::cerr << "Error: OLLAMA_HOST cannot be empty" << std::endl;
    ok = false;
  }
  if (c.model.empty()) {
    std::cerr << "Error: OLLAMA_MODEL cannot be empty" << std::endl;
    ok = false;
  }
  if (c.timeout <= 0) {
    std::cerr << "Error: timeout must be positive" << std::endl;
    ok = false;
  }
  if (c.exec_timeout <= 0) {
    std::cerr << "Error: exec_timeout must be positive" << std::endl;
    ok = false;
  }
  try {
    int p = std::stoi(c.port);
    if (p < 0 || p > 65535) {
      std::cerr << "Error: OLLAMA_PORT must be between 0 and 65535" << std::endl;
      ok = false;
    }
  } catch (...) {
    std::cerr << "Error: OLLAMA_PORT must be a valid number" << std::endl;
    ok = false;
  }

  if (!ok) {
    exit(1);
  }
}

/** Load config from environment variables, overriding defaults */
// todo: reduce complexity of load_env
// NOLINTNEXTLINE(readability-function-size)
Config load_env(const Config& defaults) {
  Config c = defaults;
  std::string val;
  if (env_get("LLAMA_PROVIDER", val)) {
    c.provider = val;
  }
  if (env_get("OLLAMA_HOST", val)) {
    // Support OLLAMA_HOST=host:port (Ollama's own convention)
    // Split on last colon to separate host from port
    auto colon = val.rfind(':');
    if (colon != std::string::npos) {
      c.host = val.substr(0, colon);
      c.port = val.substr(colon + 1);
    } else {
      c.host = val;
    }
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
  if (std::getenv("NO_COLOR")) {
    c.no_color = true;
  }
  if (std::getenv("TRACE")) {
    // Any value enables trace (TRACE=1, TRACE=true, etc.)
    c.trace = true;
  }
  return c;
}

/** Pointer-to-member type for Config string fields */
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

/** Table of string CLI options */
static const OptDef opts[] = {
    {"--provider=", nullptr, &Config::provider},
    {"--host=", "-h", &Config::host},
    {"--port=", "-p", &Config::port},
    {"--model=", "-m", &Config::model},
};

/** Pointer-to-member type for Config int fields */
using IntConfigField = int Config::*;

/** CLI int option definition: maps long/short flag to a config int field. */
struct IntOptDef {
  /** Long option prefix, e.g. "--timeout=" */
  const char* long_prefix;
  /** Short option flag, e.g. "-t" (nullptr if none) */
  const char* short_flag;
  /** Pointer-to-member for the target Config int field */
  IntConfigField field;
};

/** Table of int CLI options */
static const IntOptDef int_opts[] = {
    {"--timeout=", "-t", &Config::timeout},
    {"--exec-timeout=", nullptr, &Config::exec_timeout},
    {"--max-output=", nullptr, &Config::max_output},
};

/** Try to match arg against a long option prefix (e.g. "--host=value").
 * Returns the value part after the prefix, or empty string if no match. */
static std::string match_long(const std::string& arg, const char* prefix) {
  std::string p(prefix);
  if (arg.rfind(p, 0) == 0) {
    return arg.substr(p.size());
  }
  return "";
}

/** Try to match arg against string option definitions.
 * Checks both long form (--host=value) and short form (-h value). */
static bool match_string_opts(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  for (const auto& opt : opts) {
    std::string val = match_long(arg, opt.long_prefix);
    if (val.empty() && opt.short_flag && arg == opt.short_flag && i + 1 < argc) {
      val = argv[++i];
    }
    if (!val.empty()) {
      c.*opt.field = val;
      return true;
    }
  }
  return false;
}

/** Try to match arg against int option definitions.
 * Same as match_string_opts but converts the value to int via stoi. */
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

/// Extract the value for --files from CLI args, or empty if not a --files arg
static std::string get_files_value(const std::string& arg, int& i, int argc, const char* const argv[]) {
  std::string val = match_long(arg, "--files=");
  if (!val.empty()) {
    return val;
  }
  if (arg == "--files" && i + 1 < argc) {
    return std::string(argv[++i]);
  }
  return "";
}

/// Parse --files flag and populate config.files (ADR-030)
static void parse_files_flag(const std::string& arg, int& i, int argc, const char* const argv[], Config& c) {
  std::string files_val = get_files_value(arg, i, argc, argv);
  if (files_val.empty() && arg.rfind("--files", 0) == 0) {
    // --files or --files= with no value
    std::cerr << "Error: --files flag requires at least one file path." << std::endl;
    std::exit(1);
  }
  if (files_val.empty()) {
    return;  // not a --files arg at all
  }

  // Parse space-separated file paths
  std::istringstream iss(files_val);
  std::string path;
  while (iss >> path) {
    c.files.push_back(path);
  }
  // Activate sync mode when files are provided (ADR-005)
  if (!c.files.empty()) {
    c.mode = Mode::Sync;
  }
}

/** Load config from CLI arguments, overriding base config
 * Parses long (--key=value), short (-x value), and positional args */
Config load_cli(int argc, const char* const argv[], const Config& base) {
  Config c = base;

  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (match_opts(arg, i, argc, argv, c)) {
      continue;
    }
    if (arg == "--no-color") {
      c.no_color = true;
      continue;
    }
    if (arg == "--why-so-serious") {
      c.bofh = true;
      continue;
    }
    if (arg == "--repl") {
      c.force_repl = true;
      continue;
    }

    // --files=FILE or --files FILE — single arg, space-separated paths (ADR-030)
    parse_files_flag(arg, i, argc, argv, c);

    // Positional arg = prompt (first non-option argument)
    // Triggers sync mode per ADR-005
    if (arg[0] != '-' && c.prompt.empty()) {
      c.prompt = arg;
      c.mode = Mode::Sync;
    }
  }
  return c;
}

/** Full config resolution: defaults -> env -> .env -> cli
 * Also fills Config::instance() for global access */
Config load_config(int argc, const char* const argv[]) {
  Config c = load_env();
  load_dotenv(".env", c);
  c = load_cli(argc, argv, c);
  validate_config(c);
  Config::instance() = c;
  return c;
}
