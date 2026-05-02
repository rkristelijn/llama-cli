// Application configuration for the Ollama server with customizable settings.
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

/// Default model name — "auto" picks first available from Ollama server
constexpr const char* default_model = "auto";

/// Application configuration (singleton — use Config::instance() for global access)
struct Config {
  /// Get the global config instance
  static Config& instance() {
    static Config cfg;
    return cfg;
  }

  bool auto_confirm_write = false;                         ///< Auto-confirm file writes in sync mode (--auto-confirm-write)
  std::string provider = "ollama";                         ///< LLM provider name (ollama, mock, etc.)
  std::string host = "localhost";                          ///< Ollama server hostname
  std::string port = "11434";                              ///< Ollama server port
  std::vector<std::string> hosts;                          ///< Discovered Ollama hosts (OLLAMA_HOSTS, /scan)
  std::string model = default_model;                       ///< LLM model name
  int timeout = 120;                                       ///< HTTP request timeout in seconds
  int exec_timeout = 30;                                   ///< Max seconds for command execution
  int max_output = 10000;                                  ///< Max chars of command output for LLM context
  int max_history = 0;                                     ///< Max message pairs to keep (0=unlimited, sliding window)
  bool no_color = false;                                   ///< Disable colored output (--no-color, NO_COLOR)
  bool no_banner = false;                                  ///< Suppress ASCII banner (--no-banner, LLAMA_NO_BANNER)
  bool bofh = false;                                       ///< BOFH mode: sarcastic spinner messages (--why-so-serious)
  bool trace = false;                                      ///< Trace mode: show HTTP calls and debug info
  bool warmup = true;                                      ///< Warm up model on startup/switch (LLAMA_WARMUP)
  bool force_repl = false;                                 ///< Force REPL mode even when stdin is not a TTY (--repl)
  Mode mode = Mode::Interactive;                           ///< Execution mode (interactive or sync)
  std::string prompt_color = "yellow";                     ///< Prompt color name (LLAMA_PROMPT_COLOR)
  std::string ai_color = "purple";                         ///< AI response color name (LLAMA_AI_COLOR)
  std::string prompt;                                      ///< One-shot prompt for sync mode
  std::vector<std::string> files;                          ///< Input files for sync mode (--files, ADR-030)
  std::string session_path;                                ///< Session file for multi-turn sync (--session, ADR-056)
  std::string capabilities;                                ///< Capability flags: read,write,exec (--capabilities, ADR-056)
  std::string sandbox = ".";                               ///< Sandbox root for file ops (--sandbox, ADR-056)
  bool system_prompt_override = false;                     ///< True if user explicitly set --system-prompt
  bool allow_web_search = false;                           ///< Enable web search tool (--web-search, ADR-057)
  std::string search_url = "http://localhost:8888";        ///< SearXNG-compatible JSON API base URL (ADR-057)
  std::string search_lang = "en-US";                       ///< Search language, e.g. en-US, nl-NL (ADR-057)
  std::string search_location;                             ///< Search location, e.g. "Eindhoven, NL" (ADR-057)
  std::string memory_path = ".cache/llama-memory.md";      ///< Memory file (ADR-059)
  std::string preferences_path = ".preferences/style.md";  ///< Preferences file (ADR-059)
  std::string system_prompt =                              ///< System prompt for conversation context
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
      "Just include the tags directly in your response. "
      "Always put your explanation BEFORE the tags, never after. "
      "IMPORTANT: Never claim code contains specific patterns, functions, "
      "or design choices unless you have read the actual file content in "
      "this conversation. If you have not read a file, say so instead of "
      "guessing. Do not give scores or ratings without measurable criteria.";
  /// Web search tool prompt fragment — appended when allow_web_search is true
  static constexpr const char* web_search_prompt =
      " You can search the web with <search>query</search>. "
      "ALWAYS search BEFORE answering when the topic involves recent events, "
      "real-time data, news, current affairs, or anything beyond your training "
      "cutoff. Do NOT guess or hallucinate — search first, then answer based "
      "on the results. Keep search queries short and factual (e.g. "
      "'Trump latest news', not 'Trump news April 2026'). Never include "
      "future dates in queries. Search results are fed back to you "
      "automatically.";
};

// Load .env file into config (project-local, overrides env vars)
void load_dotenv(const std::string& path, Config& c);

// Load config from environment variables, overriding defaults
Config load_env(const Config& defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char* const argv[], const Config& base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char* const argv[]);

// Print default .env file content to stdout
void print_default_env();

#endif
