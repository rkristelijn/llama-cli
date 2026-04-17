# Llama CLI

[![Release](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/release.yml?branch=main&label=Release)](https://github.com/rkristelijn/llama-cli/releases)
[![Build](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&label=Build)](https://github.com/rkristelijn/llama-cli/actions)
[![Tests](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=test&label=Tests)](https://github.com/rkristelijn/llama-cli/actions)
[![Coverage](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=coverage&label=Coverage)](https://github.com/rkristelijn/llama-cli/actions)
[![Semgrep](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=semgrep&label=Semgrep)](https://github.com/rkristelijn/llama-cli/actions)
[![Gitleaks](https://img.shields.io/github/actions/workflow/status/rkristelijn/llama-cli/ci.yml?branch=main&event=push&job=gitleaks&label=Gitleaks)](https://github.com/rkristelijn/llama-cli/actions)

A local AI assistant in your terminal. Chat with LLMs, attach files, run commands — all offline, all private.

## Why

- **Private**: everything runs locally, no data leaves your machine
- **Fast**: no API latency, no rate limits, no subscriptions
- **Integrated**: read files, execute commands, and get AI-powered answers in one place

## Install

### Pre-built binary (recommended)

Download the latest release from [GitHub Releases](https://github.com/ollama/llama-cli/releases):

```bash
# Linux/macOS
curl -L https://github.com/ollama/llama-cli/releases/latest/download/llama-cli -o llama-cli
chmod +x llama-cli
./llama-cli
```

Or move it to your PATH:

```bash
sudo mv llama-cli /usr/local/bin/
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
> hello                          # chat with the LLM
> !ls src/                       # run command, output to terminal
> !!cat src/main.cpp             # run command, output as LLM context
> what does this code do?        # LLM can now see the file
> /set                           # show runtime options
> /set bofh                      # toggle BOFH spinner mode
> /version                       # show version info
> /help                          # show available commands
> exit                           # quit
```

The LLM can also propose commands and file writes:
```
> fix the bug in main.cpp
[proposed: write src/main.cpp]
Write to src/main.cpp? [y/n/s]
```

The LLM can read files and make targeted edits:
```
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

CLI flags override env vars, env vars override defaults. Use `--default-env > .env` to generate a template configuration file.

See [docs/user-guide.md](docs/user-guide.md) for detailed configuration options.

## Roadmap

- [x] Connect to Ollama
- [x] Configurable host, port, model, timeout
- [x] Interactive chat with conversation memory
- [x] Write files from response (`<write>`)
- [x] Targeted edits (`<str_replace>`)
- [x] Smart file reading (`<read>` with line ranges and search)
- [x] Run commands (`!`, `!!`, `<exec>`)
- [x] TUI: ANSI colors, markdown rendering, spinner
- [x] Arrow key history (linenoise)
- [x] Runtime options (`/set markdown`, `/set color`, `/set bofh`)
- [x] Ctrl+C interrupt during LLM calls
- [x] Stdin pipe support (`--files`)
- [x] Auto-diff preview before file writes
- [ ] Streaming responses
- [ ] Inline code rendering in markdown

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and [docs/](docs/README.md) for architecture decisions.
