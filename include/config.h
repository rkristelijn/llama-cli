// config.h — Application configuration
// Loads settings from defaults, environment variables, and CLI arguments.
// Precedence: CLI args > env vars > defaults

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Application configuration
struct Config {
    std::string host = "localhost";
    std::string port = "11434";
    std::string model = "gemma4:e4b";
    int timeout = 120;
};

// Load config from environment variables, overriding defaults
Config load_env(const Config &defaults = Config{});

// Load config from CLI arguments, overriding base config
Config load_cli(int argc, char *argv[], const Config &base = Config{});

// Full config resolution: defaults -> env -> cli
Config load_config(int argc, char *argv[]);

#endif
