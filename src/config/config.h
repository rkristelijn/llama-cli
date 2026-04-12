// config.h — Application configuration
// Loads settings from defaults, environment variables, and CLI arguments.
// Precedence: CLI args > env vars > defaults (ADR-004)
// CLI interface follows POSIX conventions (ADR-007)

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
 * Application configuration container for the CLI.
 *
 * Holds runtime settings for the Ollama server, chosen LLM model, timeouts,
 * execution behavior, and user/system prompts. Fields are initialized with
 * sensible defaults and are intended to be overridden by environment variables
 * or command-line arguments.
 *
 * @note `max_output` caps the number of characters of command output included
 *       in the LLM context.
 * @note `exec_timeout` limits how many seconds a spawned shell command may run.
 * @note `no_color` disables colored terminal output; `bofh` enables sarcastic
 *       spinner messages. `mode` selects interactive vs. synchronous behavior.
 */
enum class Mode { Interactive, Sync };

/// Application configuration
struct Config {
  std::string host = "localhost";    ///< Ollama server hostname
  std::string port = "11434";        ///< Ollama server port
  std::string model = "gemma4:e4b";  ///< LLM model name
  int timeout = 120;                 ///< HTTP request timeout in seconds
  int exec_timeout = 30;             ///< Max seconds for command execution
  int max_output = 10000;            ///< Max chars of command output for LLM context
  bool no_color = false;             ///< Disable colored output (--no-color, NO_COLOR)
  bool bofh = false;                 ///< BOFH mode: sarcastic spinner messages (--why-so-serious)
  Mode mode = Mode::Interactive;     ///< Execution mode (interactive or sync)
  std::string prompt;                ///< One-shot prompt for sync mode
  std::string system_prompt =        ///< System prompt for conversation context
      "You are llama-cli, a local AI assistant running in a terminal. "
      "Keep responses concise and relevant. "
      "You can run shell commands with <exec>command</exec> — use this "
      "proactively "
      "to explore files, find information, and verify your work. "
      "When asked about files, use <exec>find . -name 'pattern'</exec> or "
      "<exec>cat path</exec> to look. "
      "When asked to create or modify a file, FIRST read it with <exec>cat "
      "path</exec>, "
      "then use <write file=\"path\">full content</write> with the changes "
      "applied. "
      "NEVER write a file without reading it first if it might already exist. "
      "The user will confirm before execution. Output is fed back to you. "
      "When you receive command output, ANALYZE it — do not repeat it "
      "verbatim. "
      "Do NOT ask for confirmation or file paths — the client handles that. "
      "Just include the tags directly in your response.";
};

// Load config from environment variables, overriding defaults
Config load_env(const Config& defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char* const argv[], const Config& base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]);

#endif
