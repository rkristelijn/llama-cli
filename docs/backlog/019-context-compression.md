# 019: Context Compression

*Status*: 🔧 Partial (sliding window) · *Date*: 2026-04-19 · *Issue*: [#83](https://github.com/rkristelijn/llama-cli/issues/83)

## Problem

Conversation history grows unbounded. Long sessions burn tokens re-sending old context that the LLM has already processed. Context window overflow causes silent truncation or errors.

## Implemented: Sliding Window

Configurable `LLAMA_MAX_HISTORY` (or `--max-history=N`) trims old messages, keeping system prompts + last N message pairs. Default: 0 (unlimited). Old messages are still in the event log for reference.

## Future: LLM-based Compression

Compress conversation history while preserving key information:

1. **Keep recent** — last 3 user/assistant exchanges in full
2. **Summarize old** — compress earlier messages into a brief summary
3. **Extract key facts** — decisions, file paths, variable names mentioned
4. **Drop noise** — remove verbose exec output, repeated content

### Implementation sketch

```text
Before compression (12 messages, ~8k tokens):
  [system] [user1] [assistant1] [user2] [assistant2] ... [user6] [assistant6]

After compression (~3k tokens):
  [system]
  [summary: "User asked to fix bug in repl.cpp line 42. Identified null pointer. Applied str_replace fix."]
  [user5] [assistant5] [user6] [assistant6]
```

### Trigger

- Compress when total context exceeds 75% of model's context window
- Or after every N exchanges (configurable)

### Research

- [Dynamic Compressing Prompts (LLM-DCP)](https://arxiv.org/html/2504.11004) — task-agnostic prompt compression
- CRISP framework: keep Context, Role, Instruction; compress Specification history
- Key insight: increased token usage has drastically diminishing performance returns

## References

- [ADR-027: Event Logging](../adr/adr-027-event-logging.md) — log compression events
- [ADR-028: Execution Limits](../adr/adr-028-execution-limits.md) — max_iterations already limits loops
