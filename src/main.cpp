// llama-cli — A TUI for your llama with file interaction
// Connects to a local Ollama instance and sends prompts to a language model.
// See docs/ollama-setup.md for setup instructions.
// See docs/adr-004-configuration.md for config precedence.

#include "config.h"
#include <httplib.h>  // HTTP client (cpp-httplib via FetchContent)
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    // Load config: defaults -> env vars -> CLI args
    // See ADR-004 for precedence chain
    Config cfg = load_config(argc, argv);
    std::string url = "http://" + cfg.host + ":" + cfg.port;

    httplib::Client cli(url);
    cli.set_read_timeout(cfg.timeout);

    // POST /api/generate with stream:false waits for the full response.
    // TODO: implement streaming for real-time token output
    std::string body = R"({"model": ")" + cfg.model +
        R"(", "prompt": "What is the Ultimate Question of Life, the Universe, and Everything, if the answer is 42?", "stream": false})";

    std::cout << "Connecting to " << url << " with model " << cfg.model << "...\n\n";

    auto res = cli.Post("/api/generate", body, "application/json");
    if (res) {
        // Parse the "response" field from Ollama's JSON output.
        // Using manual parsing to avoid adding a JSON library dependency.
        auto pos = res->body.find("\"response\":\"");
        if (pos != std::string::npos) {
            pos += 12;  // skip past "response":"
            std::string output;
            for (size_t i = pos; i < res->body.size(); i++) {
                if (res->body[i] == '"' && res->body[i - 1] != '\\') break;
                // Handle JSON escape sequences
                if (res->body[i] == '\\' && i + 1 < res->body.size()) {
                    char next = res->body[i + 1];
                    if (next == 'n') { output += '\n'; i++; continue; }
                    if (next == '"') { output += '"'; i++; continue; }
                    if (next == '\\') { output += '\\'; i++; continue; }
                }
                output += res->body[i];
            }
            std::cout << output << "\n";
        }
    } else {
        std::cerr << "Error: could not connect to Ollama at " << url << "\n";
    }
    return 0;
}
