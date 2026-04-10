# Ollama Setup

This project connects to a local [Ollama](https://ollama.com) instance.

## Prerequisites

```bash
brew install ollama
brew services start ollama
ollama pull gemma4:e4b
```

## Verify

```bash
curl http://localhost:11434/api/tags
```

Should return a JSON list of installed models.

## Models

| Model | RAM | Use case |
|---|---|---|
| `gemma4:e4b` | ~10 GB | Fast, lightweight |
| `gemma4:26b` | ~16 GB | Better quality, heavier |

With 32 GB RAM: run one model at a time. Use `ollama ps` to check loaded models, `ollama stop <model>` to free memory.

## API

Ollama runs on `http://localhost:11434`. Key endpoints:

- `GET /api/tags` — list models
- `POST /api/generate` — generate completion
- `POST /api/chat` — chat completion (streaming)

### Example: generate

```bash
curl http://localhost:11434/api/generate -d '{
  "model": "gemma4:e4b",
  "prompt": "Hello!",
  "stream": false
}'
```

## Network access (optional)

To expose Ollama on your local network:

```bash
launchctl setenv OLLAMA_HOST 0.0.0.0
brew services restart ollama
```

Then accessible at `http://<your-ip>:11434` from any device on your wifi.
