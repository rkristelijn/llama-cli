// llama-cli — A local AI assistant in your terminal
// See docs/ollama-setup.md for setup instructions.
// See docs/adr-004-configuration.md for config precedence.
// See docs/adr-005-execution-modes.md for execution modes.

#include "config.h"
#include "ollama.h"
#include <iostream>

int main(int argc, char *argv[]) {
    // Load config: defaults -> env vars -> CLI args (ADR-004)
    Config cfg = load_config(argc, argv);

    // TODO: detect execution mode (interactive/sync/async) per ADR-005
    // For now, hardcoded prompt until interactive mode is implemented
    std::string prompt = "What is the Ultimate Question of Life, the Universe, and Everything, if the answer is 42?";

    std::cout << "Connecting to " << cfg.host << ":" << cfg.port
              << " with model " << cfg.model << "...\n\n";

    std::string response = ollama_generate(cfg, prompt);
    if (!response.empty()) {
        std::cout << response << "\n";
    }
    return 0;
}
