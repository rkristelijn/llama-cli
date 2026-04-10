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
make run
```

## Configuration

| Setting | CLI arg | Env var | Default |
|---------|---------|---------|---------|
| Host | `--host` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `OLLAMA_TIMEOUT` | `120` |

## Roadmap

- [x] Connect to Ollama
- [x] Configurable host, port, model, timeout
- [x] Interactive chat with conversation memory
- [x] Read files as context (`/read`)
- [ ] Streaming responses
- [ ] Write files from response
- [ ] Run commands and use output as context

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and [docs/](docs/README.md) for architecture decisions.
