// config.h — Application configuration
// Loads settings from defaults, environment variables, and CLI arguments.
// Precedence: CLI args > env vars > defaults (ADR-004)
// CLI interface follows POSIX conventions (ADR-007)

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Execution mode (ADR-005)
enum class Mode { Interactive, Sync };

/// Application configuration
struct Config {
  std::string host = "localhost";    ///< Ollama server hostname
  std::string port = "11434";        ///< Ollama server port
  std::string model = "gemma4:e4b";  ///< LLM model name
  int timeout = 120;                 ///< HTTP request timeout in seconds
  int exec_timeout = 30;             ///< Max seconds for command execution
  int max_output = 10000;            ///< Max chars of command output for LLM context
  Mode mode = Mode::Interactive;     ///< Execution mode (interactive or sync)
  std::string prompt;                ///< One-shot prompt for sync mode
  std::string system_prompt =        ///< System prompt for conversation context
      "You are llama-cli, a local AI assistant running in a terminal. "
      "Keep responses concise and relevant. "
      "When asked to create or modify a file, ALWAYS use "
      "<write file=\"path\">content</write> immediately. "
      "When you need to run a command, use <exec>command</exec>. "
      "The user will confirm before execution. Output is fed back to you. "
      "Do NOT ask for confirmation — the client handles that. "
      "Just include the tags directly in your response.";
};

// Load config from environment variables, overriding defaults
Config load_env(const Config& defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char* const argv[], const Config& base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]);

#endif
