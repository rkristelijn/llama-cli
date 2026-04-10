# Llama CLI

A C++ TUI for your local llama with file interaction.

## Quick start

```bash
brew install ollama
brew services start ollama
ollama pull gemma4:e4b
make run
```

## Configuration

| Setting | CLI arg | Env var | Default |
|---------|---------|---------|---------|
| Host | `--host` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `OLLAMA_TIMEOUT` | `120` |

CLI args override env vars, env vars override defaults.

## Development

```bash
make           # build
make run       # build and run
make test      # unit tests + comment ratio
make check     # cppcheck + semgrep + gitleaks
make install   # install git hooks
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for workflow and [docs/](docs/README.md) for architecture decisions.

## Roadmap

1. ~~Connect to Ollama~~ ✓
2. Ask question and receive answer (streaming)
3. Ability to upload file
4. Ability to run a command and receive the output

## Requirements

- C++17
- SOLID patterns
- TDD
