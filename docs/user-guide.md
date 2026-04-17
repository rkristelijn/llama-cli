---
summary: User guide for llama-cli — installation, usage, and configuration
tags: [user-guide, usage, configuration, examples]
---
# User Guide

## Installation

```bash
git clone https://github.com/rkristelijn/llama-cli
cd llama-cli
make setup         # installs all dependencies + git hooks
sudo make install  # build and install to /usr/local/bin
llama-cli --help   # verify
```

To use a different model or host:
```bash
llama-cli --model=llama3.2
llama-cli --host=192.168.1.10 --model=mistral
OLLAMA_MODEL=llama3.2 llama-cli
```

To list available models: `ollama list`  
To pull a new model: `ollama pull llama3.2`

## Usage

### Interactive mode — chat with memory

```bash
llama-cli
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
llama-cli "explain what a Makefile does"
```

Response goes to stdout, status to stderr. Pipeable:

```bash
llama-cli "generate a .gitignore for C++" > .gitignore
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

### Local Configuration (.env)

You can create a `.env` file in your project directory to persist your settings:

```bash
# Generate a template .env file
llama-cli --default-env > .env
```

Edit the `.env` file to uncomment and change the values you need. `llama-cli` will automatically load it from the current directory.

## Prerequisites

- [Ollama](https://ollama.com) running locally or on a reachable host
- At least one model pulled: `ollama pull gemma4:e4b`

See [ollama-setup.md](ollama-setup.md) for detailed setup instructions.
