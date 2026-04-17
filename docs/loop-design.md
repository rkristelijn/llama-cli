# REPL Loop Design

## Overview

The REPL loop consists of three layers:

```text
┌─────────────────────────────────────────────────────────────┐
│  run_repl() — outer loop                                    │
│  - Reads input (linenoise or getline)                       │
│  - Dispatches to command/exec/prompt handlers               │
│  - Returns when user types 'exit' or EOF                    │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  dispatch() — input parser                                  │
│  - Exit: return false to outer loop                         │
│  - Command (/help, /set, etc): handle and continue          │
│  - Exec (!, !!): run and continue                           │
│  - Prompt: call send_prompt()                               │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  send_prompt() — LLM interaction                            │
│  1. Add user message to history                             │
│  2. Call LLM with spinner                                   │
│  3. Add assistant response to history                       │
│  4. handle_response() — parse annotations                   │
│  5. If <exec> proposed and user confirmed:                  │
│     - needs_followup = true                                 │
│     - Call LLM again (follow-up)                            │
└─────────────────────────────────────────────────────────────┘
```

## The Follow-Up Loop

When the LLM proposes an `<exec>` command and the user confirms it:

```text
User: "run the tests"
LLM: "I'll run the tests: <exec>make test</exec>"
User: y
→ exec runs, output added to history
→ needs_followup = true
→ LLM called again with test output as context
LLM: "Tests passed. Now let me fix the bug: <exec>sed -i '...' src/main.cpp</exec>"
User: y
→ exec runs, output added to history
→ needs_followup = true
→ LLM called again
LLM: "Let me verify: <exec>make test</exec>"
...
```text

## Problem: Unbounded Follow-Up

The current implementation has no limit on follow-up calls. This can result in:

1. **Infinite loop** — LLM keeps proposing exec commands
2. **Token overflow** — history grows indefinitely
3. **Cost spiral** — each follow-up costs tokens
4. **No timeout** — loop runs until LLM stops responding

## Current Safeguards

| Safeguard | Status | Limitation |
|-----------|--------|------------|
| `exec_timeout` | ✅ | Kills individual commands, not the loop |
| `max_output` | ✅ | Truncates command output, not loop iterations |
| Ctrl+C | ✅ | Manual interrupt, user must notice |
| None | ❌ | No automatic loop detection or limit |

## Proposed Safeguards (ADR-028)

| Limit | Default | Behavior |
|-------|---------|----------|
| `max_iterations` | 50 | Total LLM calls per REPL session |
| `max_depth` | 10 | Nested exec/tool calls |
| `session_timeout` | 600s | Max session duration |

## Trace Mode

Enable with `--trace` for debug output:

```

[TRACE] dispatch: prompt "run the tests"
[TRACE] send_prompt: iteration=1
[TRACE] handle_response: needs_followup=true
[TRACE] send_prompt: iteration=2
[TRACE] handle_response: needs_followup=true
[TRACE] send_prompt: iteration=3
...
[TRACE] max_iterations reached (50), stopping

```text

## Debugging with Event Logs

When the loop hangs or behaves unexpectedly, use the event log to trace what happened:

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

See `scripts/dev/log-viewer.sh` and ADR-027 for details.

## Related

- @see docs/adr/adr-012-interactive-repl.md — REPL architecture
- @see docs/adr/adr-015-command-execution.md — exec annotations
- @see docs/adr/adr-027-event-logging.md — event logging for debugging
- @see docs/adr/adr-028-execution-limits.md — proposed limits
