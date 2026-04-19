# Llama CLI

[![Release](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/release.yml?branch=main&label=Release)](https://github.com/rkristelijn/llama-cli/releases)
[![Build](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&label=Build)](https://github.com/rkristelijn/llama-cli/actions)
[![Tests](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=test&label=Tests)](https://github.com/rkristelijn/llama-cli/actions)
[![Coverage](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=coverage&label=Coverage)](https://github.com/rkristelijn/llama-cli/actions)
[![Semgrep](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=semgrep&label=Semgrep)](https://github.com/rkristelijn/llama-cli/actions)
[![Gitleaks](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=gitleaks&label=Gitleaks)](https://github.com/rkristelijn/llama-cli/actions)

A local AI assistant in your terminal. Chat with LLMs, attach files, run commands — all offline, all private.

```text
> what is the eisenhower matrix?

The Eisenhower Matrix helps prioritize tasks by urgency and importance:

|                 | Urgent              | Not Urgent                  |
|-----------------|---------------------|-----------------------------|
| Important       | DO — crisis, deadlines | PLAN — growth, strategy  |
| Not Important   | DELEGATE — interrupts  | ELIMINATE — distractions |

> !!cat src/main.cpp
> explain this code and fix the bug on line 42
[proposed: str_replace src/main.cpp]
- old code
+ fixed code
Apply? [y/n]
```

Streaming responses, markdown rendering (tables, code blocks, bold, links), file I/O, command execution — all running locally on your hardware.

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

### From source

```bash
brew install ollama
brew services start ollama
ollama pull gemma4:e4b
make setup
sudo make install
```

## Usage

```bash
hello                          # chat with the LLM
!ls src/                       # run command, output to terminal
!!cat src/main.cpp             # run command, output as LLM context
what does this code do?        # LLM can now see the file
/set                           # show runtime options
/set bofh                      # toggle BOFH spinner mode
/version                       # show version info
/help                          # show available commands
exit                           # quit
```

The LLM can also propose commands and file writes:

```text
> fix the bug in main.cpp
[proposed: write src/main.cpp]
Write to src/main.cpp? [y/n/s]
```

The LLM can read files and make targeted edits:

```text
> what does repl.cpp do?
[read src/repl/repl.cpp]          # LLM reads the requested file

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

## Roadmap

Based on [ADR-050](docs/adr/adr-050-reality-check-roadmap.md) — prioritized by usability and differentiation.

### Priority 1 — Core UX

| # | Feature | Status | Backlog |
|---|---------|--------|---------|
| 1 | Streaming responses | ✅ Done | [005](docs/backlog/005-streaming.md) |
| 2 | Inline code / markdown rendering | ✅ Done | [031](docs/backlog/031-inline-code-rendering.md) |
| 3 | Fix release pipeline | 🔧 In progress | [034](docs/backlog/034-fix-release.md) |
| 4 | Tab autocompletion | ⬚ Planned | [033](docs/backlog/033-tab-autocompletion.md) |

### Priority 2 — Developer Experience

| # | Feature | Backlog |
|---|---------|---------|
| 5 | Context compression | [019](docs/backlog/019-context-compression.md) |
| 6 | Prompt templates | [021](docs/backlog/021-prompt-templates.md) |
| 7 | Exec output tuning | [007](docs/backlog/007-exec-output-tuning.md) |
| 8 | Smart confirmation | [004](docs/backlog/004-smart-confirmation.md) |

### Priority 3 — Robustness

| # | Feature | Backlog |
|---|---------|---------|
| 9 | Execution sandbox | [016](docs/backlog/016-execution-sandbox.md) |
| 10 | Command permissions | [010](docs/backlog/010-command-permissions.md) |
| 11 | Coverage bump 55→60% | [027](docs/backlog/027-coverage-bump.md) |
| 12 | Reduce complexity | [018](docs/backlog/018-reduce-complexity.md) |

### Priority 4 — Future Differentiation

| # | Feature | Backlog |
|---|---------|---------|
| 13 | Provider abstraction | [014](docs/backlog/014-provider-abstraction.md) |
| 14 | Planner/executor | [015](docs/backlog/015-planner-executor.md) |
| 15 | Distributed Ollama | [006](docs/backlog/006-distributed-ollama.md) |
| 16 | Multi-agent | [017](docs/backlog/017-multi-agent.md) |

> **Make the local AI assistant work so well that the cloud alternative isn't worth the privacy trade-off.**

## Quality Framework

This project follows [ADR-048](docs/adr/adr-048-quality-framework.md), a lean quality framework designed so that **any AI agent — including small local models — can execute development tasks** by following self-contained prompts with exact file paths, code, and verification commands.

The framework uses CMMI-inspired maturity levels (0-3) where every automated check serves at least two purposes. Current level: **CMMI 0** (working toward CMMI 1). See [ADR-048 §15](docs/adr/adr-048-quality-framework.md#15-current-status--audit-2026-04-18) for the live audit.

Task prompts for AI agents live in [`docs/prompts/`](docs/prompts/) — each prompt is copy-paste ready for a local Ollama model.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and [docs/](docs/README.md) for architecture decisions.
