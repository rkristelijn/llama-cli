/**
 * @file help.h
 * @brief Central help text — single source of truth for --help and /help
 * output.
 */
#pragma once

#include <string>

#include "config/config.h"

/**
 * CLI help text shown for `--help`.
 *
 * Contains usage, option descriptions including environment variable names and
 * defaults, examples, and notes for listing and pulling models.
 */

/**
 * REPL help text shown for `/help`.
 *
 * Lists available REPL commands with brief descriptions (command execution
 * modes, conversation controls, option toggles, version, and exit commands).
 */
namespace help {

/// CLI usage shown by --help
inline std::string cli() {
  return std::string(
             "Usage: llama-cli [OPTIONS] [PROMPT]\n"
             "\n"
             "Options:\n"
             "  --provider=NAME   LLM provider (ollama, mock) (env: LLAMA_PROVIDER, default: ollama)\n"
             "  --host=HOST       Ollama server hostname  (env: OLLAMA_HOST,    "
             "default: localhost)\n"
             "  --port=PORT       Ollama server port      (env: OLLAMA_PORT,    "
             "default: 11434)\n"
             "  --model=MODEL     LLM model name          (env: OLLAMA_MODEL,   "
             "default: ") +
         DEFAULT_MODEL +
         std::string(
             ")\n"
             "  --timeout=SECS    HTTP timeout in seconds (env: OLLAMA_TIMEOUT, "
             "default: 120)\n"
             "  --no-color        Disable colored output  (env: NO_COLOR)\n"
             "  --why-so-serious  Enable BOFH spinner mode\n"
             "  --default-env     Print .env template to stdout (use > .env to save)\n"
             "  --files=FILE      Read file(s) as context (quoted for multiple)\n"
             "  --session=PATH    Session file for multi-turn sync mode (ADR-056)\n"
             "  --capabilities=X  Allowed actions: read,write,exec (default: none)\n"
             "  --sandbox=PATH    Restrict file ops to this directory (default: .)\n"
             "  PROMPT            Run in sync mode (non-interactive)\n"
             "\n"
             "Examples:\n"
             "  llama-cli                        # interactive REPL\n"
             "  llama-cli --model=llama3.2       # use a different model\n"
             "  llama-cli --host=192.168.1.10    # remote Ollama server\n"
             "  OLLAMA_MODEL=mistral llama-cli   # via environment variable\n"
             "  llama-cli \"explain this error\"   # one-shot sync mode\n"
             "  llama-cli --files doc.md \"summarize\"  # file as context\n"
             "  cat log.txt | llama-cli \"explain\"     # stdin as context\n"
             "\n"
             "To list available models: ollama list\n"
             "To pull a model:          ollama pull llama3.2\n");
}

/// REPL commands shown by /help
constexpr const char* repl =
    "Commands:\n"
    "  !command      Run command, output to terminal\n"
    "  !!command     Run command, output as LLM context\n"
    "  /clear        Clear conversation history\n"
    "  /model        Select a model from the server\n"
    "  /color        Set prompt or AI response color\n"
    "  /set          Show options\n"
    "  /set <opt>    Toggle option (markdown, color, bofh, trace)\n"
    "  /mem          Show memories (/mem add <fact>, /mem clear)\n"
    "  /pref         Show preferences (/pref add <pref>, /pref clear)\n"
    "  /rate         Rate last response (/rate last +/-, /rate list)\n"
    "  /copy         Copy last response to clipboard\n"
    "  /version      Show version info\n"
    "  /help         Show this help\n"
    "  exit, quit    Exit the REPL\n"
    "\n"
    "Rating: After each response, press y/n/s/Enter to rate.\n"
    "  y (+) = good, n (-) = bad, s = save for review\n"
    "\n"
    "The LLM can use these tools (you confirm before apply):\n"
    "  <write file=\"path\">       Write/create a file (auto-diff shown)\n"
    "  <str_replace path=\"...\">  Targeted edit in existing file\n"
    "  <read path=\"...\" .../>    Read file (lines, search, or full)\n"
    "  <exec>command</exec>      Run a shell command\n";

}  // namespace help
