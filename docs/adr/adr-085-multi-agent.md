# ADR-085: Multi-Agent Support

*Status*: Partially Implemented · *Date*: 2026-05-05 · *Issues*: [#56](https://github.com/rkristelijn/llama-cli/issues/56), [#58](https://github.com/rkristelijn/llama-cli/issues/58)

> **Update (2026-05-07)**: Implemented via front-LLM async delegation (`<delegate>` annotation).
> The front LLM proposes subtasks, user approves, tasks run in background.
> Heuristic routing (ADR-079) removed. See ADR-099.

## Context

Single LLM loop limits reasoning quality. No way to chain specialized models or roles. Complex tasks benefit from decomposition across agents with different strengths.

## Decision

Introduce agent roles:

- **Planner** — task decomposition (large model)
- **Executor** — command generation (fast model)
- **Reviewer** — output validation (capable model)

### Flow

```text
Planner → Executor → Reviewer → Executor (loop)
```

- Route prompts to different providers/models per role
- Share context selectively between agents
- Support background execution, task interruption and resume

## Consequences

- Better reasoning quality through specialization
- Reduced cost (cheap models for simple tasks)
- More robust workflows with review loops
- Significant architectural complexity

## References

- [ADR-020: Provider Abstraction](adr-020-provider-abstraction.md)
- [ADR-079: Auto-Routing](adr-079-auto-routing.md)
- [ADR-084: Planner/Executor Separation](adr-084-planner-executor.md)
