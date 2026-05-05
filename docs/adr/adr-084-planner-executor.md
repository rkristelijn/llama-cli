# ADR-084: Planner / Executor Separation

*Status*: Proposed · *Date*: 2026-05-05 · *Issues*: [#53](https://github.com/rkristelijn/llama-cli/issues/53), [#54](https://github.com/rkristelijn/llama-cli/issues/54)

## Context

The LLM directly triggers execution through annotations. This creates tight coupling and unsafe execution paths. There is no structured task representation between planning and execution.

## Decision

Introduce strict separation:

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

## Consequences

- Safer execution model with validation between steps
- Deterministic, logged execution
- Enables multi-model workflows (cheap model plans, capable model executes)
- Prevents uncontrolled command loops

## References

- [ADR-014: Tool Annotations](adr-014-tool-annotations.md)
- [ADR-020: Provider Abstraction](adr-020-provider-abstraction.md)
- [ADR-085: Multi-Agent Support](adr-085-multi-agent.md)
