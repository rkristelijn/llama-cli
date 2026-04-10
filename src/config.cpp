// config.cpp — Configuration loading implementation
// Precedence: CLI args > env vars > defaults
// Supports --long=value, --long value, -x value, and positional args

#include "config.h"
#include <cstdlib>
#include <string>

// Load config from environment variables, overriding defaults
Config load_env(const Config &defaults) {
    Config c = defaults;
    const char *val;
    if ((val = std::getenv("OLLAMA_HOST")))    c.host = val;
    if ((val = std::getenv("OLLAMA_PORT")))    c.port = val;
    if ((val = std::getenv("OLLAMA_MODEL")))   c.model = val;
    if ((val = std::getenv("OLLAMA_TIMEOUT"))) c.timeout = std::stoi(val);
    return c;
}

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, const char * const argv[], const Config &base) {
    Config c = base;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        // Long options: --key=value
        if (arg.rfind("--host=", 0) == 0)       { c.host = arg.substr(7); continue; }
        if (arg.rfind("--port=", 0) == 0)       { c.port = arg.substr(7); continue; }
        if (arg.rfind("--model=", 0) == 0)      { c.model = arg.substr(8); continue; }
        if (arg.rfind("--timeout=", 0) == 0)    { c.timeout = std::stoi(arg.substr(10)); continue; }

        // Short options: -x value (next arg)
        if (i + 1 < argc) {
            if (arg == "-h") { c.host = argv[++i]; continue; }
            if (arg == "-p") { c.port = argv[++i]; continue; }
            if (arg == "-m") { c.model = argv[++i]; continue; }
            if (arg == "-t") { c.timeout = std::stoi(argv[++i]); continue; }
        }

        // Positional arg = prompt (first non-option argument)
        // Triggers sync mode per ADR-005
        if (arg[0] != '-' && c.prompt.empty()) {
            c.prompt = arg;
            c.mode = Mode::Sync;
        }
    }
    return c;
}

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char * const argv[]) {
    Config c = load_env();
    return load_cli(argc, argv, c);
}
