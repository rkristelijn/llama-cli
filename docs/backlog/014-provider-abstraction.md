# 014: Provider Abstraction

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#52](https://github.com/rkristelijn/llama-cli/issues/52)

## Problem

Tightly coupled to Ollama. Can't use other backends (Gemini, OpenAI, Claude) or mix providers per task.

## Idea

Abstract LLM interface:

- `LLMProvider::generate(prompt, context)`
- `LLMProvider::stream(prompt)`
- `ProviderRouter` selects provider by role (planner/executor/reviewer)

### Providers

- Ollama (local, current)
- Gemini CLI (external process)
- OpenAI / Claude (future)

### Benefits

- Hybrid workflows (local + cloud)
- Prepares for planner/executor architecture
- Reduces Ollama lock-in

### References

- [ADR-020: Provider Abstraction](../adr/adr-020-provider-abstraction.md)
- [015 — Planner/executor](015-planner-executor.md)
