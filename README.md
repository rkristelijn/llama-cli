# Llama CLI

[![CI](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&label=CI)](https://github.com/rkristelijn/llama-cli/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/codecov/c/github/rkristelijn/llama-cli?label=Coverage)](https://codecov.io/gh/rkristelijn/llama-cli)
[![Release](https://img.shields.io/github/v/release/rkristelijn/llama-cli?label=Release)](https://github.com/rkristelijn/llama-cli/releases/latest)
[![License](https://img.shields.io/github/license/rkristelijn/llama-cli)](LICENSE)
[![Tests](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/rkristelijn/a190f75bd7e08ea08f9c16f90571213f/raw/tests.json)](https://github.com/rkristelijn/llama-cli/actions/workflows/ci.yml?query=job%3Aunit-test)
[![Features](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/rkristelijn/a190f75bd7e08ea08f9c16f90571213f/raw/features.json)](https://github.com/rkristelijn/llama-cli/actions/workflows/ci.yml?query=job%3Afeature-coverage)
[![CMMI](https://img.shields.io/badge/CMMI-Level_3-brightgreen)](docs/adr/adr-048-quality-framework.md)
[![Inclusivity](https://img.shields.io/badge/Inclusivity-0_failures-brightgreen)](scripts/lint/check-inclusivity.sh)
[![Licenses](https://img.shields.io/badge/Licenses-4%2F4_permissive-brightgreen)](scripts/lint/check-licenses.sh)
[![Deps](https://img.shields.io/badge/Dependencies-4-blue)](CMakeLists.txt)

A local AI assistant in your terminal. Chat with LLMs, attach files, run commands, edit code — all offline, all private.

![llama-cli demo](docs/features/feature-tour.gif)

## Features

See [full feature demos](./docs/features/README.md) for all examples.

- **Streaming chat** with any Ollama model (Gemma, Llama, Mistral, Qwen, etc.)
- **Markdown rendering** — tables, code blocks with syntax highlighting, bold, links
- **File I/O** — LLM can read files, write new files, and make targeted `str_replace` edits
- **Command execution** — run shell commands, pipe output as LLM context (`!!cmd`)
- **Web search** — local SearXNG integration for real-time information
- **Multi-provider** — Ollama, OpenAI-compatible APIs, switchable at runtime
- **Agents/personas** — switch personality and permissions (`/agent bofh`, `/agent architect`)
- **Smart routing** — auto-select model by prompt complexity (`/auto`)
- **Vision** — attach images for multimodal models (`/image photo.png`)
- **Context compression** — summarize and compact long conversations (`/compress`)
- **Chat persistence** — save/load conversations (`/chat save`, `/chat load`)
- **Themes** — color themes for the terminal UI (`/theme hacker`)
- **Tab completion** — autocomplete commands, file paths, model names
- **Fully offline** — no cloud, no API keys, no telemetry

## Why

- **Private**: everything runs locally, no data leaves your machine
- **Fast**: no API latency, no rate limits, no subscriptions
- **Integrated**: read files, execute commands, and get AI-powered answers in one place

## Install

### One-liner (recommended)

```bash
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash
```

This auto-detects your OS and architecture (Linux x64/arm64, macOS arm64), downloads the release, verifies the checksum, and installs to `/usr/local/bin`.

Options:

```bash
# Install to a custom directory
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | INSTALL_DIR=~/.local/bin bash

# Install a specific version
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | VERSION=0.18.1 bash
```

### Future

```bash
# Below is not working yet, still on beta release, use above install instructions for a preview
brew install ollama-cli
apt install ollama-cli
```

## Usage

```bash
hello                          # chat with the LLM
!ls src/                       # run command, output to terminal
!!cat src/main.cpp             # run command, output as LLM context
what does this code do?        # LLM can now see the file
/model                         # switch model (persisted to .env)
/provider                      # list and switch providers
/agent bofh                    # switch personality (monk, architect, etc.)
/auto                          # toggle smart routing by complexity
/theme hacker                  # switch color theme
/chat save myproject           # save conversation
/chat load myproject           # restore conversation
/image photo.png               # attach image for vision models
/nick gius                     # set your prompt name
/compress                      # summarize and compact history
/usage                         # show session stats
/set                           # show runtime options
/version                       # show version info
/help                          # show available commands
exit                           # quit
```

The LLM can propose commands and file writes:

```text
> fix the bug in main.cpp
[proposed: write src/main.cpp]
Write to src/main.cpp? [y/n/s]
```

The LLM can read files and make targeted edits:

```text
> what does repl.cpp do?
[read src/repl/repl.cpp]

> fix the typo on line 42
[proposed: str_replace src/repl/repl.cpp]
- old text
+ new text
Apply str_replace to src/repl/repl.cpp? [y/n]
```

## Configuration

| Setting | CLI arg | Env var | Default |
|---------|---------|---------|---------|
| Host | `--host` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `OLLAMA_TIMEOUT` | `120` |
| Exec timeout | `--exec-timeout` | `LLAMA_EXEC_TIMEOUT` | `30` |
| Max output | `--max-output` | `LLAMA_MAX_OUTPUT` | `10000` |
| No color | `--no-color` | `NO_COLOR` | auto-detect TTY |
| BOFH mode | `--why-so-serious` | — | `false` |
| System prompt | — | `OLLAMA_SYSTEM_PROMPT` | (built-in) |

CLI flags override `.env`, `.env` overrides environment variables, which override defaults. Use `--default-env > .env` to generate a template configuration file.

See [docs/user-guide.md](docs/user-guide.md) for detailed configuration options.

## Demos

See [docs/features/](docs/features/README.md) for all animated feature demos.

> **Make the local AI assistant work so well that the cloud alternative isn't worth the privacy trade-off.**

## Quality Framework

This project follows [ADR-048](docs/adr/adr-048-quality-framework.md), a lean quality framework designed so that **any AI agent — including small local models — can execute development tasks** by following self-contained prompts with exact file paths, code, and verification commands.

The framework uses CMMI-inspired maturity levels (0-3) where every automated check serves at least two purposes. Current level: **CMMI 3** (Defined). See [ADR-048 §15](docs/adr/adr-048-quality-framework.md#15-current-status--audit-2026-04-24) for the live audit.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and [docs/](docs/README.md) for architecture decisions.
