// config.h — Application configuration
// Loads settings from defaults, environment variables, and CLI arguments.
// Precedence: CLI args > env vars > defaults (ADR-004)
// CLI interface follows POSIX conventions (ADR-007)

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

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

/// Application configuration (singleton — use Config::instance() for global access)
struct Config {
  /// Get the global config instance
  static Config& instance() {
    static Config cfg;
    return cfg;
  }

  std::string provider = "ollama";   ///< LLM provider name (ollama, mock, etc.)
  std::string host = "localhost";    ///< Ollama server hostname
  std::string port = "11434";        ///< Ollama server port
  std::string model = "gemma4:e4b";  ///< LLM model name
  int timeout = 120;                 ///< HTTP request timeout in seconds
  int exec_timeout = 30;             ///< Max seconds for command execution
  int max_output = 10000;            ///< Max chars of command output for LLM context
  bool no_color = false;             ///< Disable colored output (--no-color, NO_COLOR)
  bool bofh = false;                 ///< BOFH mode: sarcastic spinner messages (--why-so-serious)
  bool trace = false;                ///< Trace mode: show HTTP calls and debug info
  bool force_repl = false;           ///< Force REPL mode even when stdin is not a TTY (--repl)
  Mode mode = Mode::Interactive;     ///< Execution mode (interactive or sync)
  std::string prompt;                ///< One-shot prompt for sync mode
  std::vector<std::string> files;    ///< Input files for sync mode (--files, ADR-030)
  std::string system_prompt =        ///< System prompt for conversation context
      "You are llama-cli, a local AI assistant running in a terminal. "
      "Keep responses concise and relevant. Always respond in the same "
      "language as the user's message. "
      "You can run shell commands with <exec>command</exec> — use this "
      "proactively "
      "to explore files, find information, and verify your work. "
      "To read a file, use <read path=\"file\" lines=\"10-20\"/> for a line "
      "range, "
      "<read path=\"file\" search=\"term\"/> to search, or <read "
      "path=\"file\"/> for the full file. "
      "To modify an existing file, prefer <str_replace "
      "path=\"file\"><old>exact old text</old><new>replacement</new>"
      "</str_replace> — this is safer than rewriting the whole file. "
      "Use <write file=\"path\">full content</write> only for new files. "
      "The user will confirm before any write. Output is fed back to you. "
      "When you receive command output, ANALYZE it — do not repeat it "
      "verbatim. "
      "Do NOT ask for confirmation or file paths — the client handles that. "
      "Just include the tags directly in your response.";
};

// Load .env file into config (project-local, overrides env vars)
void load_dotenv(const std::string& path, Config& c);

// Load config from environment variables, overriding defaults
Config load_env(const Config& defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char* const argv[], const Config& base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]);

#endif
