# 011: Runtime Model Switching

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#48](https://github.com/rkristelijn/llama-cli/issues/48)

## Problem

Switching models requires restarting the CLI with a different `--model` flag.

## Idea

- `/model` — show current model + list available (via Ollama `/api/tags`)
- `/model <name>` — switch mid-session, preserve history
- Tab-completion for model names

### References

- [ADR-004: Configuration](../adr/adr-004-configuration.md)
- [ADR-049: Model Selection Command](../adr/adr-049-model-selection-command.md)
