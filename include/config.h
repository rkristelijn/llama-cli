// config.h — Application configuration
// Loads settings from defaults, environment variables, and CLI arguments.
// Precedence: CLI args > env vars > defaults (ADR-004)
// CLI interface follows POSIX conventions (ADR-007)

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Execution mode (ADR-005)
enum class Mode { Interactive, Sync };

// Application configuration
struct Config {
  std::string host = "localhost";
  std::string port = "11434";
  std::string model = "gemma4:e4b";
  int timeout = 120;
  Mode mode = Mode::Interactive;
  std::string prompt;
  std::string system_prompt =
      "You are llama-cli, a local AI assistant running in a terminal. "
      "Keep responses concise and relevant. "
      "The user can load files with /read <file>. "
      "Files appear as [file: path] followed by their contents. "
      "When asked to create or modify a file, wrap content in "
      "<write file=\"path\">content</write>. "
      "The user will confirm before any file is written.";
};

// Load config from environment variables, overriding defaults
Config load_env(const Config& defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char* const argv[], const Config& base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]);

#endif
