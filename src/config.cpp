// config.cpp — Configuration loading implementation
// Precedence: CLI args > env vars > defaults

#include "config.h"
#include <cstdlib>
#include <cstring>
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
// Expects --key=value format
Config load_cli(int argc, const char * const argv[], const Config &base) {
    Config c = base;
    const std::string prefix_host = "--host=";
    const std::string prefix_port = "--port=";
    const std::string prefix_model = "--model=";
    const std::string prefix_timeout = "--timeout=";

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.rfind(prefix_host, 0) == 0)         c.host = arg.substr(prefix_host.size());
        else if (arg.rfind(prefix_port, 0) == 0)    c.port = arg.substr(prefix_port.size());
        else if (arg.rfind(prefix_model, 0) == 0)   c.model = arg.substr(prefix_model.size());
        else if (arg.rfind(prefix_timeout, 0) == 0)  c.timeout = std::stoi(arg.substr(prefix_timeout.size()));
    }
    return c;
}

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, const char * const argv[]) {
    Config c = load_env();
    return load_cli(argc, argv, c);
}
