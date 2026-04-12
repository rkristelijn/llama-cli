/**
 * @file help.h
 * @brief Central help text — single source of truth for --help and /help
 * output.
 */
#pragma once

namespace help {

/// CLI usage shown by --help
constexpr const char* cli =
    "Usage: llama-cli [OPTIONS] [PROMPT]\n"
    "\n"
    "Options:\n"
    "  --host=HOST       Ollama server hostname  (env: OLLAMA_HOST,    "
    "default: localhost)\n"
    "  --port=PORT       Ollama server port      (env: OLLAMA_PORT,    "
    "default: 11434)\n"
    "  --model=MODEL     LLM model name          (env: OLLAMA_MODEL,   "
    "default: gemma4:e4b)\n"
    "  --timeout=SECS    HTTP timeout in seconds (env: OLLAMA_TIMEOUT, "
    "default: 120)\n"
    "  --no-color        Disable colored output  (env: NO_COLOR)\n"
    "  --why-so-serious  Enable BOFH spinner mode\n"
    "  PROMPT            Run in sync mode (non-interactive)\n"
    "\n"
    "Examples:\n"
    "  llama-cli                        # interactive REPL\n"
    "  llama-cli --model=llama3.2       # use a different model\n"
    "  llama-cli --host=192.168.1.10    # remote Ollama server\n"
    "  OLLAMA_MODEL=mistral llama-cli   # via environment variable\n"
    "  llama-cli \"explain this error\"   # one-shot sync mode\n"
    "\n"
    "To list available models: ollama list\n"
    "To pull a model:          ollama pull llama3.2\n";

/// REPL commands shown by /help
constexpr const char* repl =
    "Commands:\n"
    "  !command      Run command, output to terminal\n"
    "  !!command     Run command, output as LLM context\n"
    "  /clear        Clear conversation history\n"
    "  /set          Show options\n"
    "  /set <opt>    Toggle option (markdown, color, bofh)\n"
    "  /version      Show version info\n"
    "  /help         Show this help\n"
    "  exit, quit    Exit the REPL\n";

}  // namespace help
