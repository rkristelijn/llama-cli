// test_config.cpp — Unit tests for config loading
// Tests the precedence chain: defaults -> env vars -> CLI args

#include "config.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

// Helper to set env var portably
static void set_env(const char *name, const char *value) {
    setenv(name, value, 1);
}

// Helper to unset env var
static void unset_env(const char *name) {
    unsetenv(name);
}

// Clean all OLLAMA_ env vars before each test
static void clean_env() {
    unset_env("OLLAMA_HOST");
    unset_env("OLLAMA_PORT");
    unset_env("OLLAMA_MODEL");
    unset_env("OLLAMA_TIMEOUT");
}

// Test: defaults are correct
static void test_defaults() {
    Config c;
    assert(c.host == "localhost");
    assert(c.port == "11434");
    assert(c.model == "gemma4:e4b");
    assert(c.timeout == 120);
    std::cout << "PASS: test_defaults\n";
}

// Test: env vars override defaults
static void test_env_overrides_defaults() {
    clean_env();
    set_env("OLLAMA_HOST", "192.168.1.10");
    set_env("OLLAMA_PORT", "9999");
    set_env("OLLAMA_MODEL", "gemma4:26b");
    set_env("OLLAMA_TIMEOUT", "60");

    Config c = load_env();
    assert(c.host == "192.168.1.10");
    assert(c.port == "9999");
    assert(c.model == "gemma4:26b");
    assert(c.timeout == 60);

    clean_env();
    std::cout << "PASS: test_env_overrides_defaults\n";
}

// Test: unset env vars keep defaults
static void test_env_keeps_defaults() {
    clean_env();

    Config c = load_env();
    assert(c.host == "localhost");
    assert(c.port == "11434");
    assert(c.model == "gemma4:e4b");
    assert(c.timeout == 120);

    std::cout << "PASS: test_env_keeps_defaults\n";
}

// Test: CLI args override base config
static void test_cli_overrides() {
    const char *argv[] = {"llama-cli", "--host=10.0.0.1", "--model=gemma4:26b", nullptr};

    Config c = load_cli(3, argv);
    assert(c.host == "10.0.0.1");
    assert(c.model == "gemma4:26b");
    // port and timeout should remain defaults
    assert(c.port == "11434");
    assert(c.timeout == 120);

    std::cout << "PASS: test_cli_overrides\n";
}

// Test: short flags override base config
static void test_short_flags() {
    const char *argv[] = {"llama-cli", "-h", "10.0.0.1", "-m", "gemma4:26b", "-p", "9999", "-t", "60", nullptr};

    Config c = load_cli(9, argv);
    assert(c.host == "10.0.0.1");
    assert(c.model == "gemma4:26b");
    assert(c.port == "9999");
    assert(c.timeout == 60);

    std::cout << "PASS: test_short_flags\n";
}

// Test: positional arg becomes prompt
static void test_positional_prompt() {
    const char *argv[] = {"llama-cli", "explain this error", nullptr};

    Config c = load_cli(2, argv);
    assert(c.prompt == "explain this error");
    assert(c.mode == Mode::Sync);

    std::cout << "PASS: test_positional_prompt\n";
}

// Test: no args means interactive mode
static void test_interactive_mode() {
    const char *argv[] = {"llama-cli", nullptr};

    Config c = load_cli(1, argv);
    assert(c.prompt.empty());
    assert(c.mode == Mode::Interactive);

    std::cout << "PASS: test_interactive_mode\n";
}

// Test: options + positional arg
static void test_options_with_prompt() {
    const char *argv[] = {"llama-cli", "--model=gemma4:26b", "review this code", nullptr};

    Config c = load_cli(3, argv);
    assert(c.model == "gemma4:26b");
    assert(c.prompt == "review this code");
    assert(c.mode == Mode::Sync);

    std::cout << "PASS: test_options_with_prompt\n";
}

// Test: CLI args override env vars (full chain)
static void test_cli_overrides_env() {
    clean_env();
    set_env("OLLAMA_HOST", "192.168.1.10");
    set_env("OLLAMA_MODEL", "gemma4:26b");

    const char *argv[] = {"llama-cli", "--host=localhost", nullptr};

    Config c = load_config(2, argv);
    // CLI wins over env for host
    assert(c.host == "localhost");
    // env wins over default for model (no CLI override)
    assert(c.model == "gemma4:26b");

    clean_env();
    std::cout << "PASS: test_cli_overrides_env\n";
}

// Test: unknown CLI args are ignored
static void test_unknown_args_ignored() {
    const char *argv[] = {"llama-cli", "--unknown=value", nullptr};

    Config c = load_cli(2, argv);
    assert(c.host == "localhost");

    std::cout << "PASS: test_unknown_args_ignored\n";
}

int main() {
    test_defaults();
    test_env_overrides_defaults();
    test_env_keeps_defaults();
    test_cli_overrides();
    test_short_flags();
    test_positional_prompt();
    test_interactive_mode();
    test_options_with_prompt();
    test_cli_overrides_env();
    test_unknown_args_ignored();

    std::cout << "\nAll config tests passed.\n";
    return 0;
}
