/**
 * @file main.cpp
 * @brief Entry point: loads config, dispatches to sync or interactive mode.
 * @see docs/adr/adr-005-execution-modes.md
 * @see docs/adr/adr-007-cli-interface.md
 */

#include <iostream>
#include <string>

#include "config.h"
#include "ollama.h"
#include "repl.h"
#include "tui.h"

/** Entry point: loads config, dispatches to sync or interactive mode. */
int main(int argc, char* argv[]) {
  // Load config: defaults -> env vars -> CLI args (ADR-004)
  Config cfg = load_config(argc, const_cast<const char* const*>(argv));
  bool color = tui::use_color(cfg.no_color);

  if (cfg.mode == Mode::Sync) {
    // Sync mode: one-shot, response to stdout (ADR-007)
    tui::system_msg(std::cerr, color,
                    "Connecting to " + cfg.host + ":" + cfg.port + " with model " + cfg.model + "...");
    std::string response = ollama_generate(cfg, cfg.prompt);
    if (!response.empty()) {
      std::cout << response << "\n";
    }
  } else {
    // Interactive mode: REPL loop (ADR-012)
    tui::system_msg(std::cerr, color, "llama-cli — connected to " + cfg.host + ":" + cfg.port + " (" + cfg.model + ")");
    tui::system_msg(std::cerr, color, "Type your prompt. 'exit' or Ctrl+D to quit.\n");
    auto generate = [&cfg](const std::vector<Message>& msgs) { return ollama_chat(cfg, msgs); };
    run_repl(generate, cfg);
  }
  return 0;
}
