# Ollama API & CLI

Ollama provides a local API for running and interacting with LLMs programmatically.

## Base URL

- Local: `http://localhost:11434/api`
- Cloud: `https://ollama.com/api`

## CLI Commands

```bash
ollama pull <model>           # Download a model
ollama run <model>            # Run a model interactively
ollama list                   # List installed models
ollama show <model>           # Show model details
ollama rm <model>             # Remove a model
ollama serve                  # Start the API server
```

## API Endpoints

### Generate

Generate a response from a prompt.

```text
POST /api/generate
```

**Request:**

```json
{
  "model": "gemma3",
  "prompt": "Why is the sky blue?",
  "stream": true,
  "system": "You are a helpful assistant",
  "format": "json",
  "options": {
    "temperature": 0.7,
    "top_p": 0.9
  }
}
```

**Response:**

```json
{
  "model": "gemma3",
  "created_at": "2023-11-07T05:31:56Z",
  "response": "The sky appears blue because...",
  "done": true,
  "total_duration": 5043500291,
  "load_duration": 1234567,
  "prompt_eval_count": 12,
  "eval_count": 42
}
```

### Chat

Generate a chat message in a conversation.

```text
POST /api/chat
```

**Request:**

```json
{
  "model": "gemma3",
  "messages": [
    {
      "role": "user",
      "content": "Why is the sky blue?"
    }
  ],
  "stream": true,
  "format": "json"
}
```

**Response:**

```json
{
  "model": "gemma3",
  "created_at": "2023-11-07T05:31:56Z",
  "message": {
    "role": "assistant",
    "content": "The sky appears blue because..."
  },
  "done": true,
  "total_duration": 5043500291,
  "eval_count": 42
}
```

### List Models

List available models and their details.

```text
GET /api/tags
```

**Response:**

```json
{
  "models": [
    {
      "name": "gemma3",
      "model": "gemma3",
      "modified_at": "2025-10-03T23:34:03Z",
      "size": 3338801804,
      "digest": "a2af6cc3eb7fa8be8504abaf9b04e88f17a119ec3f04a3addf55f92841195f5a",
      "details": {
        "format": "gguf",
        "family": "gemma",
        "parameter_size": "4.3B",
        "quantization_level": "Q4_K_M"
      }
    }
  ]
}
```

### Show Model Details

Get detailed information about a specific model.

```text
POST /api/show
```

**Request:**

```json
{
  "name": "gemma3"
}
```

### Embeddings

Create vector embeddings for text.

```text
POST /api/embed
```

**Request:**

```json
{
  "model": "nomic-embed-text",
  "input": "The sky is blue"
}
```

### Running Models

List currently running models.

```text
GET /api/ps
```

### Pull Model

Download a model from the registry.

```text
POST /api/pull
```

**Request:**

```json
{
  "name": "gemma3",
  "stream": true
}
```

### Delete Model

Remove a model.

```text
DELETE /api/delete
```

**Request:**

```json
{
  "name": "gemma3"
}
```

### Version

Get Ollama version.

```text
GET /api/version
```

## Common Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `model` | string | Model name (required) |
| `stream` | boolean | Stream responses (default: true) |
| `format` | string/object | Output format: `json` or JSON schema |
| `keep_alive` | string/number | Keep model loaded (e.g., `5m`, `0` to unload) |
| `temperature` | float | Randomness (0-1, default: 0.7) |
| `top_p` | float | Nucleus sampling (0-1, default: 0.9) |
| `top_k` | integer | Top-k sampling |
| `num_predict` | integer | Max tokens to generate |

## Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `model` | string | Model name used |
| `created_at` | string | ISO 8601 timestamp |
| `done` | boolean | Generation complete |
| `total_duration` | integer | Total time in nanoseconds |
| `load_duration` | integer | Model load time in nanoseconds |
| `prompt_eval_count` | integer | Input token count |
| `eval_count` | integer | Output token count |

## Examples

### cURL

```bash
# Generate response
curl http://localhost:11434/api/generate -d '{
  "model": "gemma3",
  "prompt": "Why is the sky blue?"
}'

# Chat
curl http://localhost:11434/api/chat -d '{
  "model": "gemma3",
  "messages": [{"role": "user", "content": "Hello"}]
}'

# List models
curl http://localhost:11434/api/tags
```

### Python

```python
import requests

response = requests.post(
    "http://localhost:11434/api/generate",
    json={
        "model": "gemma3",
        "prompt": "Why is the sky blue?"
    }
)
print(response.json())
```

### JavaScript

```javascript
const response = await fetch("http://localhost:11434/api/generate", {
  method: "POST",
  body: JSON.stringify({
    model: "gemma3",
    prompt: "Why is the sky blue?"
  })
});
const data = await response.json();
console.log(data);
```

## Notes

- API is backwards compatible; versioning is not strict
- Streaming responses return newline-delimited JSON
- All timestamps are ISO 8601 format
- Durations are in nanoseconds
- Models must be pulled before use
- Official libraries: [Python](https://github.com/ollama/ollama-python), [JavaScript](https://github.com/ollama/ollama-js)
