# 020: Temperature Tuning

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#84](https://github.com/rkristelijn/llama-cli/issues/84)

## Problem

The LLM uses the same temperature for all tasks. Code generation needs precision (low randomness), while brainstorming benefits from creativity (higher randomness). One-size-fits-all wastes quality or tokens.

## Idea

Auto-detect task type from the prompt and adjust parameters:

| Task type | Temperature | Top-P | Top-K | Trigger |
|-----------|-------------|-------|-------|---------|
| Code generation | 0.1 | 0.3 | 10 | `<write>`, `<str_replace>`, code keywords |
| Analysis | 0.3 | 0.5 | 20 | "explain", "why", "review", "debug" |
| Creative | 0.8 | 0.9 | 50 | "brainstorm", "suggest", "ideas" |
| Factual | 0.0 | 1.0 | 1 | "what is", "how many", "list" |

### Configuration

```env
# Override auto-detection
OLLAMA_TEMPERATURE=0.3
# Or per-session
/set temperature 0.1
/set temperature auto   # re-enable auto-detection
```

### Implementation sketch

1. Before sending prompt, classify task type from keywords
2. Set Ollama `options.temperature`, `options.top_p`, `options.top_k`
3. Log the chosen parameters in event log for analysis

## References

- [Ollama API options](https://github.com/ollama/ollama/blob/main/docs/api.md)
- [ADR-004: Configuration](../adr/adr-004-configuration.md)
