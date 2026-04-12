---
summary: User guide for llama-cli — installation, usage, and configuration
tags: [user-guide, usage, configuration, examples]
---
# User Guide

## Installation

```bash
git clone https://github.com/rkristelijn/llama-cli
cd llama-cli
make setup    # installs all dependencies + git hooks
make          # build
```

## Usage

### Interactive mode — chat with memory

```bash
./build/llama-cli
> what is the capital of France?
Paris.
> and of Germany?
Berlin.
> exit
```

The model remembers the conversation. Type `exit`, `quit`, or Ctrl+D to stop.

### Run commands

```bash
> !ls src/                    # run command, output to terminal
> !!cat src/main.cpp          # run command, add output as LLM context
> what does this code do?     # LLM can now see the file
```

- `!command` — run and show output (not sent to LLM)
- `!!command` — run, show output, and add to LLM context
- The LLM can propose commands with `<exec>command</exec>` — you confirm with y/n

### File writes

The LLM can propose file writes:
```
> fix the bug in main.cpp
[proposed: write src/main.cpp]
Write to src/main.cpp? [y/n/s/d]
```

- `y` — write the file
- `n` — skip
- `s` — show new content, then decide
- `d` — show diff (existing files only, red=removed, green=added)

Existing files are backed up to `.bak` before overwriting.

### REPL commands

```
/set           Show runtime options
/set <opt>     Toggle option (markdown, color, bofh)
/version       Show version info
/clear         Clear conversation history
/help          Show available commands
exit, quit     Exit the REPL
Ctrl+C         Interrupt LLM call
```

### Sync mode — one-shot from command line

```bash
./build/llama-cli "explain what a Makefile does"
```

Response goes to stdout, status to stderr. Pipeable:

```bash
./build/llama-cli "generate a .gitignore for C++" > .gitignore
```

### Configuration

| Setting | Flag | Short | Env var | Default |
|---------|------|-------|---------|---------|
| Host | `--host` | `-h` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `-p` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `-m` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `-t` | `OLLAMA_TIMEOUT` | `120` |
| Exec timeout | `--exec-timeout` | — | `LLAMA_EXEC_TIMEOUT` | `30` |
| Max output | `--max-output` | — | `LLAMA_MAX_OUTPUT` | `10000` |
| No color | `--no-color` | — | `NO_COLOR` | auto-detect TTY |
| BOFH mode | `--why-so-serious` | — | — | `false` |
| System prompt | — | — | `OLLAMA_SYSTEM_PROMPT` | (built-in) |

CLI flags override env vars, env vars override defaults.

## Prerequisites

- [Ollama](https://ollama.com) running locally or on a reachable host
- At least one model pulled: `ollama pull gemma4:e4b`

See [ollama-setup.md](ollama-setup.md) for detailed setup instructions.
