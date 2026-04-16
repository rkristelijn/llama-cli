# ADR-033: kiro-cli Ollama Integration Reference

*Status*: Accepted · *Date*: 2026-04-15 · *Context*: Research on connecting kiro-cli to local Ollama instances for local AI coding assistance.

## Decision

Document kiro-cli Ollama integration options for future reference and potential implementation.

### kiro-cli Current Architecture

kiro-cli uses a provider-agnostic design with:
- `chat.defaultModel` setting (currently `minimax-m2.1`)
- ACP protocol with `--model` flag
- MCP server support for extensibility

### Integration Options

| Option | Command | Notes |
|--------|---------|-------|
| **ACP model flag** | `kiro-cli chat --model <model>` | Requires OpenAI-compatible API |
| **Settings config** | `kiro-cli settings chat.defaultModel "ollama/llama3"` | Persistent default |
| **Ollama as API** | `ollama serve` on `localhost:11434` | OpenAI-compatible endpoint |

### Ollama API Compatibility

Ollama provides an OpenAI-compatible API at `http://localhost:11434/v1/chat/completions`:

```bash
# Test Ollama API directly
curl http://localhost:11434/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3",
    "messages": [{"role": "user", "content": "Hello"}]
  }'
```

### kiro-cli Settings Example

```json
{
  "chat.defaultModel": "ollama/llama3",
  "chat.defaultAgent": null
}
```

### Alternative Local CLI Tools

For comparison, other AI coding CLIs with native Ollama support:

| Tool | Ollama Support | Notes |
|------|----------------|-------|
| **Continue** | Native | `continue.dev` — VS Code extension + CLI |
| **Opencode** | Via config | OpenAI's CLI, supports local endpoints |
| **Claude Code** | Via API | Anthropic's CLI, local proxy possible |

## Rationale

1. **Privacy**: Local inference keeps code/data on-device
2. **Cost**: Free after initial model download
3. **Offline**: Works without internet connection
4. **Custom Models**: Fine-tuned models possible

## Consequences

### Positive

- Complete privacy for sensitive code
- No per-token costs
- Custom/fine-tuned model support

### Negative

- Requires local GPU/CPU resources
- Slower inference than cloud APIs
- Model management overhead

## Configuration for Ollama + kiro-cli

```bash
# 1. Start Ollama API server
ollama serve

# 2. Set kiro-cli to use Ollama
kiro-cli settings chat.defaultModel "llama3"

# 3. Or use per-session
kiro-cli chat --model llama3
```

## References

- [Ollama OpenAI Compatibility](https://github.com/ollama/ollama/blob/main/docs/openai.md)
- kiro-cli ACP protocol: `kiro-cli acp --help`
- [Continue.dev](https://continue.dev) — Native Ollama support