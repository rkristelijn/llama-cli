# ADR-004: Configuration Strategy

## Status
Accepted

## Date
2026-04-10

## Context
The application has hardcoded values for host, port, model, and timeout. This is an anti-pattern: it prevents others from using different setups without recompiling, and makes testing harder.

## Options Considered

| Option | Pros | Cons |
|--------|------|------|
| CLI arguments only | Simple, unix-way | Verbose when many options |
| Environment variables only | Simple, 12-factor style | Not visible, easy to forget |
| Config file (JSON/YAML) | Persistent, clear overview | Extra parsing dependency |
| **Env vars + CLI args** | Flexible, layered | Slightly more code |

## Decision
Use environment variables as defaults, with CLI arguments as overrides.

### Precedence (highest wins)
1. CLI arguments (`--host`, `--model`, etc.)
2. Environment variables (`OLLAMA_HOST`, `OLLAMA_MODEL`, etc.)
3. Built-in defaults

### Configuration

| Setting | CLI arg | Env var | Default |
|---------|---------|---------|---------|
| Host | `--host` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `OLLAMA_TIMEOUT` | `120` |

### Examples

```bash
# All defaults
./build/llama-cli

# Override model via env
OLLAMA_MODEL=gemma4:26b ./build/llama-cli

# Override host via CLI (takes precedence over env)
OLLAMA_HOST=192.168.1.10 ./build/llama-cli --host=localhost
```

## Design

```mermaid
sequenceDiagram
    participant Main
    participant Defaults
    participant Env as Environment Variables
    participant CLI as CLI Arguments
    participant App

    Main->>Defaults: Load built-in defaults
    Note right of Defaults: host=localhost<br/>port=11434<br/>model=gemma4:e4b<br/>timeout=120

    Main->>Env: Read OLLAMA_HOST, OLLAMA_PORT, etc.
    Env-->>Main: Override defaults if set

    Main->>CLI: Parse --host, --port, --model, --timeout
    CLI-->>Main: Override env/defaults if set

    Main->>App: Pass final config
    App->>App: Connect and run
```

## Rationale
- Env vars align with Ollama's own `OLLAMA_HOST` convention
- Follows [12-factor app](https://12factor.net/config) factor III: store config in the environment
- CLI args are a pragmatic addition — 12-factor targets web services, but for CLI tools `--host=x` is more natural than `OLLAMA_HOST=x`
- Built-in defaults mean it works out of the box with a standard Ollama install
- Defaults must live somewhere — the anti-pattern was not having a way to override them, not their existence (SOLID: Open/Closed principle)
- No config file dependency — keeps the project lean

## Consequences
- Need to parse `argv` (no external library — use `getopt` or manual parsing)
- Need to read env vars (`std::getenv`)
- Prompt remains hardcoded for now — will become interactive input (phase 2)
