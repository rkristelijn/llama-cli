# ADR-027: Event Logging & Replay

*Status*: Proposed · *Date*: 2026-04-12 · *Context*: Issue #57 — Debug and optimize agent behavior by recording all actions in a structured event log.

## Problem

- Agent loops are hard to debug without execution history
- Token/cost usage is opaque — no per-call breakdown
- No way to replay or analyze agent decisions post-hoc

## Decision

Log all agent actions as structured JSON events.

### Event schema

```json
{
  "timestamp": "2026-04-12T18:00:00.123Z",
  "agent": "planner",
  "action": "exec",
  "input": "make test",
  "output": "[doctest] Status: SUCCESS!",
  "duration_ms": 120,
  "tokens_prompt": 150,
  "tokens_completion": 45
}
```

### Events to log

| Event | Fields |
|-------|--------|
| LLM prompts/responses | timestamp, agent, action, input, output, duration_ms, tokens_prompt, tokens_completion |
| Tool calls | timestamp, agent, action, input, output, duration_ms |
| Shell executions | timestamp, agent, action, input, output, duration_ms |
| Errors | timestamp, agent, action, input, output, duration_ms |

### Key design decisions

1. **Log token counts, not costs** — prices change, models get swapped; raw counts let you recalculate later
2. **No screenshots** — CLI tool, all output is text; TUI visualization is a future option
3. **Append-only log** — new events added to the end, never modified
4. **Local file** — `~/.llama-cli/events.jsonl` (JSONL = one JSON object per line)

### Human-readable log viewer

JSONL is not human-friendly. A log viewer script provides readable output:

```bash
make log                    # show last 50 events
make log 123                # filter events matching "123"
make log exec --context 5   # show exec events with 5 lines context
```

Output format:

```text
[18:25:03] planner   exec         |  120ms | 150 45
  input:  make test
  output: [doctest] Status: SUCCESS!
---
```

See `scripts/dev/log-viewer.sh` for implementation.

### Future: Replay viewer

Once the logging foundation is in place, a TUI-based replay viewer could:

- Scrub through execution timeline
- Filter by agent, action, or time range
- Show aggregate stats (total tokens, total time, cost estimates)

## References

- @see docs/loop-design.md — REPL loop and follow-up behavior
- @see docs/adr/adr-023-self-documenting-processes.md — self-documenting principles
- @see docs/adr/adr-015-command-execution.md — exec events are already captured
- @see scripts/dev/log-viewer.sh — human-readable log viewer
- Thanks to @m13v for feedback on issue #57:
  - [macos-session-replay](https://github.com/m13v/macos-session-replay) — reference for replay architecture
  - Tip: log token counts per-call for flexible cost calculation
