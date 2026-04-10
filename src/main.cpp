// llama-cli — A local AI assistant in your terminal
// See docs/adr-005-execution-modes.md for execution modes.
// See docs/adr-007-cli-interface.md for CLI interface design.
// See docs/adr-012-interactive-repl.md for REPL design.

#include <iostream>
#include <string>

#include "config.h"
#include "ollama.h"
#include "repl.h"

int main(int argc, char* argv[]) {
  // Load config: defaults -> env vars -> CLI args (ADR-004)
  Config cfg = load_config(argc, const_cast<const char* const*>(argv));

  if (cfg.mode == Mode::Sync) {
    // Sync mode: one-shot, response to stdout (ADR-007)
    std::cerr << "Connecting to " << cfg.host << ":" << cfg.port << " with model " << cfg.model << "...\n";
    std::string response = ollama_generate(cfg, cfg.prompt);
    if (!response.empty()) {
      std::cout << response << "\n";
    }
  } else {
    // Interactive mode: REPL loop (ADR-012)
    std::cerr << "llama-cli — connected to " << cfg.host << ":" << cfg.port << " (" << cfg.model << ")\n";
    std::cerr << "Type your prompt. 'exit' or Ctrl+D to quit.\n\n";
    auto generate = [&cfg](const std::vector<Message>& msgs) { return ollama_chat(cfg, msgs); };
    run_repl(generate, cfg.system_prompt);
  }
  return 0;
}
