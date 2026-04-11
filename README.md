# Llama CLI

A local AI assistant in your terminal. Chat with LLMs, attach files, run commands — all offline, all private.

## Why

- **Private**: everything runs locally, no data leaves your machine
- **Fast**: no API latency, no rate limits, no subscriptions
- **Integrated**: read files, execute commands, and get AI-powered answers in one place

## Quick start

```bash
brew install ollama
brew services start ollama
ollama pull gemma4:e4b
make setup
make run
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

## Roadmap

- [x] Connect to Ollama
- [x] Configurable host, port, model, timeout
- [x] Interactive chat with conversation memory
- [x] Write files from response (`<write>`)
- [x] Run commands (`!`, `!!`, `<exec>`)
- [x] TUI: ANSI colors, markdown rendering, spinner
- [x] Arrow key history (linenoise)
- [x] Runtime options (`/set markdown`, `/set color`, `/set bofh`)
- [x] Ctrl+C interrupt during LLM calls
- [ ] Streaming responses
- [ ] Stdin pipe support

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and [docs/](docs/README.md) for architecture decisions.
