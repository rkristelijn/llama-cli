---
summary: The provided text is a documentation for a command-line interface (CLI) tool called `llama-cli`. The documentation covers various aspects of the tool's architecture, startup flow, provider switching, auto-routing tiers, legacy overview, modules, request flow, and more.  Here are some key takeaways f
tags: [architecture, technical, design, modules]
---

# Architecture

## System Overview (v2 — Multi-Provider)

```text
┌─────────────────────────────────────────────────────────────────┐
│                         llama-cli                                │
├─────────────┬──────────────┬──────────────┬─────────────────────┤
│   Config    │    REPL      │   Sync Mode  │   Planner           │
│  (.env/CLI) │  (commands)  │  (one-shot)  │  (auto-routing)     │
├─────────────┴──────────────┴──────────────┴─────────────────────┤
│                    Provider Abstraction (ADR-020)                │
├──────────┬──────────┬──────────┬──────────┬─────────────────────┤
│  Ollama  │   tgpt   │  Gemini  │ kiro-cli │  MultiHost Router   │
│ Provider │ Provider │ Provider │ Provider │  (failover/match)   │
├──────────┴──────────┴──────────┴──────────┴─────────────────────┤
│                    Model Registry (ADR-081)                      │
│         Provider → Host → Model (speed, cost, capabilities)     │
├─────────────────────────────────────────────────────────────────┤
│  TUI (theme)  │  Logger (JSONL)  │  Annotations  │  Exec       │
└─────────────────────────────────────────────────────────────────┘
```

### Startup Flow

1. Load config: defaults → .env → env vars → CLI args (ADR-004)
2. Detect mode: Sync (has prompt/pipe) or Interactive (TTY)
3. Create provider: based on LLAMA_PROVIDER config
4. [REPL only] Scan providers: probe all hosts, build ModelRegistry
5. [REPL only] Auto-select model: fastest in sweetspot (11-28B)
6. Enter main loop (REPL) or execute one-shot (Sync)

### Provider Switching

| Action | What happens |
|--------|-------------|
| `/provider <name>` | Recreates provider via factory, updates Config |
| `/model <number>` | Switches host+port+model+provider from registry |
| `/auto` toggle | Enables per-prompt routing by complexity |

### Auto-Routing Tiers (ADR-079)

| Tier | Prompt type | Target |
|------|-------------|--------|
| 1 | Simple (< 30 words, factual) | Fastest 3B (jarvis/pepper) |
| 2 | Medium (question, 30-90 words) | 14B (local) |
| 3 | Complex (code, multi-part) | 27B or cloud |
| 4 | Current info (news, today) | tgpt/gemini |

## Legacy Overview

```mermaid
graph TB
    User([User]) --> Main
    Main --> Config[Config module]
    Main --> REPL[REPL loop]
    Main --> Ollama[Ollama client]
    Config --> Env[Environment vars]
    Config --> CLI[CLI arguments]
    REPL --> Ollama
    Ollama --> JSON[JSON parser]
    Ollama --> HTTP[cpp-httplib]
    HTTP --> Server([Ollama server])
```text

## Modules

| Module | Files | Responsibility |
|--------|-------|----------------|
| **main** | `src/main.cpp` | Wires config, client, and I/O. Detects execution mode. |
| **config** | `src/config/config.h`, `src/config/config.cpp` | Loads settings from defaults → env vars → CLI args |
| **ollama** | `src/ollama/ollama.h`, `src/ollama/ollama.cpp` | HTTP client for Ollama API (`/api/generate`, `/api/chat`) |
| **json** | `src/json/json.h`, `src/json/json.cpp` | Minimal JSON string extraction without external deps |
| **repl** | `src/repl/repl.h`, `src/repl/repl.cpp` | Interactive loop with conversation memory, spinner, markdown |
| **command** | `src/command/command.h`, `src/command/command.cpp` | Input parsing: !, !!, /slash, exit, prompts |
| **annotation** | `src/annotation/annotation.h`, `src/annotation/annotation.cpp` | Parse `<write>` annotations from LLM responses |
| **exec** | `src/exec/exec.h`, `src/exec/exec.cpp` | Shell command execution with timeout and output capture |
| **tui** | `src/tui/tui.h` | ANSI colors, markdown rendering, spinner (header-only) |

## Request flow

### Sync mode

```mermaid
sequenceDiagram
    participant User
    participant Main
    participant Config
    participant Ollama
    participant Server as Ollama Server

    User->>Main: llama-cli "prompt"
    Main->>Config: load_config(argc, argv)
    Config-->>Main: Config{mode=Sync, prompt="..."}
    Main->>Ollama: ollama_generate(cfg, prompt)
    Ollama->>Server: POST /api/generate
    Server-->>Ollama: {"response": "..."}
    Ollama-->>Main: response text
    Main->>User: stdout
```text

### Interactive mode

```mermaid
sequenceDiagram
    participant User
    participant REPL
    participant Ollama
    participant Server as Ollama Server

    User->>REPL: starts llama-cli (no args)
    loop conversation
        REPL->>User: "> " prompt
        User->>REPL: types message
        REPL->>REPL: append to history
        REPL->>Ollama: ollama_chat(cfg, history)
        Ollama->>Server: POST /api/chat {messages: [...]}
        Server-->>Ollama: {"message": {"content": "..."}}
        Ollama-->>REPL: response text
        REPL->>REPL: append response to history
        REPL->>User: print response
    end
    User->>REPL: "exit" / Ctrl+D
```text

## Testability

All modules are tested independently using doctest (Given/When/Then style):

| Test | What | How |
|------|------|-----|
| `test_config` | Config loading + precedence | Direct function calls with mock argv/env |
| `test_json` | JSON extraction | String in, string out |
| `test_repl` | REPL loop behavior | Injected ChatFn + string streams for I/O |
| `test_command` | Input parsing | All input types: !, !!, /slash, exit, prompts |
| `test_annotation` | Write annotation parsing | Single/multi/no/malformed annotations |
| `test_exec` | Command execution | Basic, failing, stderr, truncation, timeout |

The REPL and Ollama client are decoupled via `ChatFn` — a `std::function` that can be replaced with a mock in tests. No HTTP calls are made during testing.

## Dependencies

| Dependency | Version | How | Purpose |
|------------|---------|-----|---------|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | v0.18.7 | CMake FetchContent | HTTP client |
| [doctest](https://github.com/doctest/doctest) | v2.4.12 | CMake FetchContent | Test framework |
| [cpp-linenoise](https://github.com/yhirose/cpp-linenoise) | master | CMake FetchContent | Arrow key history, readline |

Both are downloaded automatically at build time. No manual installation needed.

## Directory structure

```text
llama-cli/
├── src/                  # Source code
│   ├── main.cpp          # Entry point, mode detection
│   ├── config.cpp        # Configuration loading
│   ├── config/           # config.h, config.cpp, config_test.cpp
│   ├── json/             # json.h, json.cpp, json_test.cpp
│   ├── ollama/           # ollama.h, ollama.cpp
│   ├── repl/             # repl.h, repl.cpp, repl_test.cpp, *_it.cpp
│   ├── command/          # command.h, command.cpp, *_test.cpp, *_it.cpp
│   ├── annotation/       # annotation.h, annotation.cpp, *_test.cpp, *_it.cpp
│   ├── exec/             # exec.h, exec.cpp, exec_test.cpp
│   ├── tui/              # tui.h (header-only), markdown_it.cpp
│   ├── main.cpp          # Entry point
│   └── test_helpers.h    # Shared test utilities
├── docs/                 # Documentation
│   ├── adr/              # Architecture Decision Records
│   ├── user-guide.md     # How to use
│   └── architecture.md   # This file
├── .config/              # Tool configs and hooks (.clang-format, .clang-tidy, Doxyfile, pre-commit)
├── scripts/              # Shell scripts (setup, build-index, test coverage, CI helpers)
└── .github/workflows/    # CI pipeline
```text
