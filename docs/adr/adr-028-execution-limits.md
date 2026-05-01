# ADR-028: Execution Limits

*Status*: Implemented · *Date*: 2026-04-12 · *Context*: Prevent agent loops from hanging indefinitely.

## Problem

- Agent loops can get stuck in infinite cycles
- No way to stop a runaway execution
- Debugging is hard without visibility

## Decision

Add configurable execution limits.

### Limits

| Limit | CLI arg | Env var | Default | Purpose |
|-------|---------|---------|---------|---------|
| Max depth | `--max-depth` | `LLAMA_MAX_DEPTH` | `10` | Max nested exec/tool calls |
| Max iterations | `--max-iterations` | `LLAMA_MAX_ITERATIONS` | `50` | Max REPL iterations per session |
| Session timeout | `--session-timeout` | `LLAMA_SESSION_TIMEOUT` | `600` | Max seconds before session expires |

### Behavior

When a limit is reached:

1. Log the event with context
2. Show a clear message to the user
3. Stop execution gracefully (not a crash)

### Trace mode

Add `--trace` flag for debug output:

```text
[TRACE] enter: handle_exec
[TRACE] exit: handle_exec (duration_ms=12)
```

Trace goes to stderr, disabled by default.

## References

- @see docs/adr/adr-027-event-logging.md — event logging for debugging
- @see docs/adr/adr-015-command-execution.md — exec module
