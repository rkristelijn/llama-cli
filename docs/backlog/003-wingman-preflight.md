# 003: Wingman — Prompt Preflight

*Status*: Idea · *Date*: 2026-04-19 · *Tag*: `feature:wingman:preflight` · *Issue*: [#81](https://github.com/rkristelijn/llama-cli/issues/81)

## Problem

Users type prompts that are vague, ambiguous, or expensive to process. The LLM attempts them anyway, burning tokens on poor results. There is no feedback loop before the request hits the model.

## Idea

A fast preflight check on the user's prompt before sending it to the LLM. Gives instant feedback on prompt quality.

### Preflight checks

| Check | Signal | Feedback |
|-------|--------|----------|
| **Fuzzy/vague** | Short prompt, no specifics | ⚠️ "Vague prompt — try adding context or a specific question" |
| **Cost prediction** | Token estimate from prompt length + context | 💰 "~2k tokens, ~3s" |
| **Missing context** | References files not in context | 📎 "Did you mean to attach `main.cpp`? Use `!!cat main.cpp` first" |
| **Too broad** | Very long context window | 🔍 "Large context — consider narrowing with line ranges" |

### Examples

```text
> fix it
⚠️ Preflight: vague prompt. What should be fixed? Try: "fix the segfault in repl.cpp line 42"

> refactor the entire codebase to use modern C++
💰 Preflight: ~15k tokens estimated. Consider breaking this into smaller tasks.

> why does main.cpp crash?
📎 Preflight: main.cpp not in context. Run !!cat src/main.cpp first?
```

### Design considerations

- **Fast** — preflight runs locally, no LLM call; rule-based heuristics
- **Non-blocking** — show feedback, let user proceed or rephrase
- **Toggle** — `/set preflight` to enable/disable
- **Short feedback loop** — response appears instantly, before LLM call starts

### Implementation sketch

1. Before sending prompt to LLM, run preflight checks
2. Token estimate: count words × ~1.3 + context tokens
3. Fuzzy detection: prompt < 5 words with no file references
4. Missing context: prompt mentions filenames not in conversation
5. Display warnings, wait for Enter to proceed or let user rephrase

## References

- [ADR-027: Event Logging](../adr/adr-027-event-logging.md) — log preflight events
- [ADR-012: Interactive REPL](../adr/adr-012-interactive-repl.md) — REPL input flow
- [001 — Log optimization](001-log-optimization.md) — prompt pattern data identifies fuzzy prompts
