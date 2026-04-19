# 015: Planner / Executor Separation

*Status*: Idea · *Date*: 2026-04-19 · *Issues*: [#53](https://github.com/rkristelijn/llama-cli/issues/53), [#54](https://github.com/rkristelijn/llama-cli/issues/54)

## Problem

The LLM directly triggers execution through annotations. This creates tight coupling and unsafe execution paths. No structured task representation.

## Idea

Strict separation:

1. **Planner** (LLM) — generates structured task plan (JSON)
2. **Executor** (kernel) — validates and executes each step

### Task schema

```json
{
  "goal": "string",
  "steps": [
    { "id": 1, "tool": "exec | file | git", "action": "string", "input": {} }
  ],
  "status": "pending | running | completed | failed"
}
```

### Flow

User request → Planner generates plan → Executor runs steps → Results fed back to planner

### Benefits

- Safer execution model
- Deterministic, logged execution
- Enables multi-model workflows
- Prevents uncontrolled command loops

### References

- [ADR-014: Tool Annotations](../adr/adr-014-tool-annotations.md)
- [014 — Provider abstraction](014-provider-abstraction.md)
- [017 — Multi-agent](017-multi-agent.md)
