// llama-cli — A local AI assistant in your terminal
// See docs/adr-005-execution-modes.md for execution modes.
// See docs/adr-007-cli-interface.md for CLI interface design.

#include "config.h"
#include "ollama.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    // Load config: defaults -> env vars -> CLI args (ADR-004)
    Config cfg = load_config(argc, const_cast<const char * const *>(argv));

    if (cfg.mode == Mode::Sync) {
        // Sync mode: one-shot, response to stdout (ADR-007)
        std::cerr << "Connecting to " << cfg.host << ":" << cfg.port
                  << " with model " << cfg.model << "...\n";
        std::string response = ollama_generate(cfg, cfg.prompt);
        if (!response.empty()) {
            std::cout << response << "\n";
        }
    } else {
        // Interactive mode: TODO implement REPL
        std::cerr << "Interactive mode not yet implemented.\n";
        std::cerr << "Usage: llama-cli \"your prompt here\"\n";
    }
    return 0;
}
