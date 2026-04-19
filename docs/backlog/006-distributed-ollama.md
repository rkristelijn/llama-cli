# 006: Distributed Ollama

*Status*: Idea · *Date*: 2026-04-19 · *Issues*: [#12](https://github.com/rkristelijn/llama-cli/issues/12), [#32](https://github.com/rkristelijn/llama-cli/issues/32)

## Problem

A single Ollama instance is limited by the hardware it runs on. Users with multiple machines (e.g. Raspberry Pi + Mac Studio) can't pool resources.

## Idea

Distribute prompts across multiple Ollama instances for parallel processing or load balancing.

### Possible approaches

- Round-robin across configured hosts
- Route by model availability (query `/api/tags` on each host)
- Route by task complexity (small model for simple tasks, large model for complex)

### Configuration sketch

```env
OLLAMA_HOSTS=pi.local:11434,studio.local:11434
OLLAMA_STRATEGY=round-robin  # or model-match, least-busy
```

### References

- [ADR-004: Configuration](../adr/adr-004-configuration.md)
