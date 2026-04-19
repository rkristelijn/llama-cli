# 002: Wingman — Command Tips

*Status*: Idea · *Date*: 2026-04-19 · *Tag*: `feature:wingman:commandtips` · *Issue*: [#80](https://github.com/rkristelijn/llama-cli/issues/80)

## Problem

Users run shell commands via `!!` but may not know optimal flags, pipes, or alternatives. The LLM sees the command output but doesn't proactively suggest better approaches.

## Idea

After a `!!` command executes, offer a short tip to optimize the command. Focus on common CLI tools: `ls`, `grep`, `awk`, `sed`, `tail`, `find`, `xargs`, pipes.

### Examples

```text
> !!grep -r "TODO" src/
[output...]
💡 Tip: grep -rn --include='*.cpp' "TODO" src/  — adds line numbers, limits to C++ files
```

```text
> !!cat log.txt | grep error | wc -l
[output...]
💡 Tip: grep -c error log.txt  — single command, no pipes needed
```

## Additional scope

When printing the commands with tags it doesn't show that nicely, just the bare command with tags. should be markedup in different color or bold

### Design considerations

- **Non-intrusive** — tips appear after output, one line, dismissable
- **Frequency** — don't tip on every command; use log data ([001](001-log-optimization.md)) to identify repeated patterns
- **Local-only** — tip generation runs through the same LLM, no external calls
- **Toggle** — `/set wingman` to enable/disable

### Implementation sketch

1. After `!!` output, send a short prompt: "Suggest one optimization for this command: `{cmd}`"
2. Constrain response to one line
3. Display with `💡 Tip:` prefix
4. Log as `action: "wingman:commandtips"` in event log

## References

- [ADR-027: Event Logging](../adr/adr-027-event-logging.md) — log wingman events
- [ADR-015: Command Execution](../adr/adr-015-command-execution.md) — `!!` command flow
- [001 — Log optimization](001-log-optimization.md) — command frequency drives tip selection
- [smart linux commands](https://www.quora.com/What-are-Linux-commands-that-improve-your-working-efficiency-significantly)
