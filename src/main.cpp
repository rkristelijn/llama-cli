#include <httplib.h>
#include <iostream>
#include <string>

int main() {
    httplib::Client cli("http://localhost:11434");
    cli.set_read_timeout(120);

    std::string body = R"({
        "model": "gemma4:e4b",
        "prompt": "What is the Ultimate Question of Life, the Universe, and Everything, if the answer is 42?",
        "stream": false
    })";

    std::cout << "Asking gemma4:e4b...\n\n";

    auto res = cli.Post("/api/generate", body, "application/json");
    if (res) {
        // Extract "response" field from JSON
        auto pos = res->body.find("\"response\":\"");
        if (pos != std::string::npos) {
            pos += 12;
            std::string output;
            for (size_t i = pos; i < res->body.size(); i++) {
                if (res->body[i] == '"' && res->body[i - 1] != '\\') break;
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
