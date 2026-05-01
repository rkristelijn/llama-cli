/**
 * @file config.cpp
 * @brief Configuration loading: defaults → env vars → CLI args.
 * @see docs/adr/adr-004-configuration.md
 * @see docs/adr/adr-015-command-execution.md
 */

#include "config/config.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/** Safe string-to-int conversion. Returns fallback on invalid input. */
static int safe_stoi(const std::string& val, int fallback) {
  try {
    return std::stoi(val);
  } catch (...) {
    std::cerr << "Warning: invalid integer '" << val << "', using default " << fallback << "\n";
    return fallback;
  }
}

/** Parse host:port string, handling IPv6 bracket notation.
 * Supports: "host:port", "[::1]:port", "host", "[::1]" */
static void parse_host_port(const std::string& val, std::string& host, std::string& port) {
  if (!val.empty() && val[0] == '[') {
    // IPv6 bracket notation: [::1]:port
    auto bracket = val.find(']');
    if (bracket != std::string::npos) {
      host = val.substr(1, bracket - 1);
      if (bracket + 2 < val.size() && val[bracket + 1] == ':') {
        port = val.substr(bracket + 2);
      }
    } else {
      // Missing closing bracket - strip leading '[' and warn
      host = val.substr(1);
      std::cerr << "Warning: malformed IPv6 address '" << val << "', missing ']'\n";
    }
  } else {
    // IPv4 or hostname: only split on colon if there's exactly one
    auto first = val.find(':');
    auto last = val.rfind(':');
    if (first != std::string::npos && first == last) {
      // Exactly one colon: host:port
      host = val.substr(0, first);
      port = val.substr(first + 1);
    } else {
      // No colon or multiple colons (bare IPv6): treat as host only
      host = val;
    }
  }
}

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
// pmccabe:skip-complexity
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

    // Strip inline comments (# outside of quotes) BEFORE removing quotes
    bool in_quotes = false;
    char quote_char = 0;
    for (size_t i = 0; i < val.size(); ++i) {
      if (!in_quotes && (val[i] == '"' || val[i] == '\'')) {
        in_quotes = true;
        quote_char = val[i];
      } else if (in_quotes && val[i] == quote_char) {
        in_quotes = false;
      } else if (!in_quotes && val[i] == '#') {
        val.erase(i);
        break;
      }
    }

    // Strip optional surrounding quotes from value
    if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
      val = val.substr(1, val.size() - 2);
    }
    // Trim trailing whitespace
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) {
      val.pop_back();
    }
    if (key == "OLLAMA_HOST") {
      // Support host:port format, including IPv6 bracket notation
      parse_host_port(val, c.host, c.port);
    } else if (key == "OLLAMA_HOSTS") {
      // Comma-separated list of host:port pairs from /scan
      c.hosts.clear();
      std::istringstream ss(val);
      std::string token;
      while (std::getline(ss, token, ',')) {
        if (!token.empty()) c.hosts.push_back(token);
      }
    } else if (key == "OLLAMA_PORT") {
      c.port = val;
    } else if (key == "OLLAMA_MODEL") {
      c.model = val;
    } else if (key == "OLLAMA_TIMEOUT") {
      c.timeout = safe_stoi(val, 120);
    } else if (key == "LLAMA_EXEC_TIMEOUT") {
      c.exec_timeout = safe_stoi(val, 30);
    } else if (key == "LLAMA_MAX_OUTPUT") {
      c.max_output = safe_stoi(val, 10000);
    } else if (key == "OLLAMA_SYSTEM_PROMPT") {
      c.system_prompt = val;
    } else if (key == "LLAMA_PROVIDER") {
      c.provider = val;
    } else if (key == "LLAMA_PROMPT_COLOR") {
      c.prompt_color = val;
    } else if (key == "LLAMA_AI_COLOR") {
      c.ai_color = val;
    } else if (key == "NO_COLOR") {
      c.no_color = true;
    } else if (key == "LLAMA_NO_BANNER") {
      c.no_banner = (val == "true" || val == "1");
    } else if (key == "TRACE") {
      // TRACE=true or TRACE=1 enables trace output (HTTP calls, timing)
      c.trace = (val == "true" || val == "1");
    } else if (key == "LLAMA_WARMUP") {
      c.warmup = (val == "true" || val == "1");
    } else if (key == "ALLOW_WEB_SEARCH") {
      c.allow_web_search = (val == "true" || val == "1");
    } else if (key == "LLAMA_SEARCH_URL") {
      c.search_url = val;
    } else if (key == "LLAMA_SEARCH_LANG") {
      c.search_lang = val;
    } else if (key == "LLAMA_SEARCH_LOCATION") {
      c.search_location = val;
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
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
Config load_env(const Config& defaults) {
  Config c = defaults;
  std::string val;
  if (env_get("LLAMA_PROVIDER", val)) {
    c.provider = val;
  }
  if (env_get("OLLAMA_HOST", val)) {
    // Support host:port format, including IPv6 bracket notation
    parse_host_port(val, c.host, c.port);
  }
  if (env_get("OLLAMA_PORT", val)) {
    c.port = val;
  }
  if (env_get("OLLAMA_MODEL", val)) {
    c.model = val;
  }
  if (env_get("OLLAMA_TIMEOUT", val)) {
    c.timeout = safe_stoi(val, 120);
  }
  if (env_get("LLAMA_EXEC_TIMEOUT", val)) {
    c.exec_timeout = safe_stoi(val, 30);
  }
  if (env_get("LLAMA_MAX_OUTPUT", val)) {
    c.max_output = safe_stoi(val, 10000);
  }
  if (env_get("OLLAMA_SYSTEM_PROMPT", val)) {
    c.system_prompt = val;
  }
  if (std::getenv("NO_COLOR")) {
    c.no_color = true;
  }
  if (std::getenv("LLAMA_NO_BANNER")) {
    c.no_banner = true;
  }
  if (std::getenv("TRACE")) {
    // Any value enables trace (TRACE=1, TRACE=true, etc.)
    c.trace = true;
  }
  if (env_get("LLAMA_WARMUP", val)) {
    c.warmup = (val == "true" || val == "1");
  }
  if (env_get("ALLOW_WEB_SEARCH", val)) {
    c.allow_web_search = (val == "true" || val == "1");
  }
  if (env_get("LLAMA_SEARCH_URL", val)) {
    c.search_url = val;
  }
  if (env_get("LLAMA_SEARCH_LANG", val)) {
    c.search_lang = val;
  }
  if (env_get("LLAMA_SEARCH_LOCATION", val)) {
    c.search_location = val;
  }
  if (env_get("LLAMA_PROMPT_COLOR", val)) {
    c.prompt_color = val;
  }
  if (env_get("LLAMA_AI_COLOR", val)) {
    c.ai_color = val;
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
    {"--system-prompt=", nullptr, &Config::system_prompt},
    {"--session=", nullptr, &Config::session_path},
    {"--capabilities=", nullptr, &Config::capabilities},
    {"--sandbox=", nullptr, &Config::sandbox},
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
      c.*opt.field = safe_stoi(val, c.*opt.field);
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
    if (arg == "--no-banner") {
      c.no_banner = true;
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
    if (arg == "--web-search") {
      c.allow_web_search = true;
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

/**
 * @brief Prints a template .env file to stdout with all default values.
 *
 * Each variable is output on its own line, commented out, and followed by a
 * brief description and example values. This output can be redirected to a
 * .env file for local configuration (e.g., `llama-cli --default-env > .env`).
 */
void print_default_env() {
  Config c;
  // Strip newlines from system prompt for the template
  std::string flat_prompt = c.system_prompt;
  size_t pos = 0;
  while ((pos = flat_prompt.find('\n', pos)) != std::string::npos) {
    flat_prompt.replace(pos, 1, " ");
  }

  std::cout << "# llama-cli configuration template. Save to .env (e.g. ./llama-cli --default-env > .env) and uncomment "
               "to use.\n"
            << "# LLAMA_PROVIDER=" << c.provider << " # LLM provider (ollama, mock)\n"
            << "# OLLAMA_HOST=" << c.host << " # Ollama server hostname (localhost, 127.0.0.1)\n"
            << "# OLLAMA_PORT=" << c.port << " # Ollama server port (11434)\n"
            << "# OLLAMA_MODEL=" << c.model << " # LLM model name (e.g. gemma4:26b, llama3.2)\n"
            << "# OLLAMA_TIMEOUT=" << c.timeout << " # HTTP timeout in seconds for LLM generation\n"
            << "# LLAMA_EXEC_TIMEOUT=" << c.exec_timeout << " # Max seconds for shell command execution\n"
            << "# LLAMA_MAX_OUTPUT=" << c.max_output << " # Max chars of command output for LLM context\n"
            << "# NO_COLOR=1 # Disable colored terminal output\n"
            << "# LLAMA_NO_BANNER=1 # Suppress ASCII banner on startup\n"
            << "# TRACE=1 # Enable trace mode (show HTTP calls and timing)\n"
            << "# LLAMA_WARMUP=0 # Warm up model on startup or switch (1=enabled, 0=disabled)\n"
            << "# ALLOW_WEB_SEARCH=1 # Enable web search tool (ADR-057)\n"
            << "# LLAMA_SEARCH_URL=" << c.search_url << " # SearXNG-compatible JSON API base URL\n"
            << "# LLAMA_SEARCH_LANG=" << c.search_lang << " # Search language (en-US, nl-NL, ...)\n"
            << "# LLAMA_SEARCH_LOCATION= # Search location (e.g. Eindhoven, NL)\n"
            << "# LLAMA_PROMPT_COLOR=" << c.prompt_color << " # Prompt color (yellow, cyan, etc.)\n"
            << "# LLAMA_AI_COLOR=" << c.ai_color << " # AI response color (purple, green, etc.)\n"
            << "# OLLAMA_SYSTEM_PROMPT=\"" << flat_prompt << "\" # System prompt\n";
}
