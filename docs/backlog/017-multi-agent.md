# 017: Multi-Agent Support

*Status*: Idea · *Date*: 2026-04-19 · *Issues*: [#56](https://github.com/rkristelijn/llama-cli/issues/56), [#58](https://github.com/rkristelijn/llama-cli/issues/58)

## Problem

Single LLM loop limits reasoning quality. No way to chain specialized models or roles.

## Idea

Introduce agent roles:

- **Planner** — task decomposition
- **Executor** — command generation
- **Reviewer** — output validation

### Flow

```text
Planner → Executor → Reviewer → Executor (loop)
```

- Route prompts to different providers/models per role
- Share context selectively between agents
- REPL becomes a command interface for an AI execution kernel
- Support background execution, task interruption and resume

### Benefits

- Better reasoning quality
- Reduced cost (cheap models for simple tasks)
- More robust workflows

### References

- [014 — Provider abstraction](014-provider-abstraction.md)
- [015 — Planner/executor](015-planner-executor.md)
- [016 — Execution sandbox](016-execution-sandbox.md)
