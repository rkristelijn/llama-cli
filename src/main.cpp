// llama-cli — A TUI for your llama with file interaction
// Connects to a local Ollama instance and sends prompts to a language model.
// See docs/ollama-setup.md for setup instructions.

#include <httplib.h>  // HTTP client (cpp-httplib via FetchContent)
#include <iostream>
#include <string>

// Ollama API endpoint
// Default: http://localhost:11434
// Docs: https://github.com/ollama/ollama/blob/main/docs/api.md
int main() {
    httplib::Client cli("http://localhost:11434");
    cli.set_read_timeout(120);  // LLM responses can be slow

    // POST /api/generate with stream:false waits for the full response.
    // TODO: implement streaming for real-time token output
    std::string body = R"({
        "model": "gemma4:e4b",
        "prompt": "What is the Ultimate Question of Life, the Universe, and Everything, if the answer is 42?",
        "stream": false
    })";

    std::cout << "Asking gemma4:e4b...\n\n";

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
        std::cerr << "Error: could not connect to Ollama on port 11434\n";
    }
    return 0;
}
