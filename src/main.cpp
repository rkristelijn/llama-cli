/**
 * @file main.cpp
 * @brief Entry point: loads config, dispatches to sync or interactive mode.
 * @see docs/adr/adr-005-execution-modes.md
 * @see docs/adr/adr-007-cli-interface.md
 */

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "config/config.h"
#include "help.h"
#include "ollama/ollama.h"
#include "repl/repl.h"
#include "tui/tui.h"

/**
 * @brief Program entry point that loads configuration and runs either a one-shot sync request or an interactive REPL.
 *
 * If the `--help` argument is present, prints the CLI help and exits immediately. Otherwise the function
 * loads configuration (defaults → environment → CLI), determines UI color, and dispatches to either
 * sync mode (produce a single response) or interactive mode (REPL).
 *
 * @return int Exit code: `0` on normal completion.
 */
// pmccabe:skip-complexity — TODO: refactor main dispatch logic
int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--help") {
      std::cout << help::cli;
      return 0;
    }
  }

  // Load config: defaults -> env vars -> CLI args (ADR-004)
  Config cfg = load_config(argc, const_cast<const char* const*>(argv));
  bool color = tui::use_color(cfg.no_color);

  // Build context from --files or stdin pipe (ADR-030)
  std::string context;
  if (!cfg.files.empty()) {
    for (const auto& path : cfg.files) {
      std::ifstream f(path);
      if (!f) {
        std::cerr << "error: cannot read " << path << "\n";
        return 1;
      }
      context += "--- " + path + " ---\n";
      context += std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      context += "\n";
    }
  } else if (!isatty(STDIN_FILENO)) {
    context = std::string((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  }
  if (!context.empty()) {
    cfg.prompt = cfg.prompt.empty() ? context : cfg.prompt + "\n\n" + context;
    cfg.mode = Mode::Sync;
  }

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
