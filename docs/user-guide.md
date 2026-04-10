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

### Sync mode — one-shot from command line

```bash
./build/llama-cli "explain what a Makefile does"
```

Response goes to stdout, status to stderr. This means it's pipeable:

```bash
# Save response to file
./build/llama-cli "generate a .gitignore for C++" > .gitignore

# Chain with other tools
./build/llama-cli "list 5 common HTTP status codes" | grep 404
```

### Remote Ollama — use another machine

```bash
./build/llama-cli --host=192.168.1.100 "hello from my laptop"
```

### Configuration

All settings can be set via CLI flags, environment variables, or left as defaults:

| Setting | Flag | Short | Env var | Default |
|---------|------|-------|---------|---------|
| Host | `--host` | `-h` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `-p` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `-m` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `-t` | `OLLAMA_TIMEOUT` | `120` |

CLI flags override env vars, env vars override defaults.

```bash
# Use a different model
./build/llama-cli -m gemma4:26b "explain quantum computing"

# Set default model via env
export OLLAMA_MODEL=gemma4:26b
./build/llama-cli "explain quantum computing"
```

## Prerequisites

- [Ollama](https://ollama.com) running locally or on a reachable host
- At least one model pulled: `ollama pull gemma4:e4b`

See [ollama-setup.md](ollama-setup.md) for detailed setup instructions.
