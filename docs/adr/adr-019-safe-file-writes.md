# ADR-019: Safe File Writes — Diff Preview and Backup

## Status
Accepted

## Date
2026-04-11

## Context
The LLM can propose file writes via `<write file="path">content</write>`. The user confirms with y/n/s. Problem: when overwriting an existing file, the user cannot see what will change. This caused real data loss (TODO.md was completely replaced by LLM output).

## Decision

### Diff preview
- Existing files: prompt shows `[y/n/s/d]` — `d` shows a line-by-line diff (red=removed, green=added)
- New files: prompt shows `[y/n/s]` — no diff needed
- `s` and `d` loop back to the prompt (user can inspect multiple times before deciding)

### Automatic backup
- Before overwriting an existing file, a `.bak` copy is created
- Backup is silent — no confirmation needed
- Allows recovery even after confirming a bad write

### Diff format
Simple line-by-line comparison (not unified diff):
```
  unchanged line
- removed line      (red)
+ added line        (green)
```

## Alternatives considered

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| No diff, just show new content (current) | Simple | Can't see what's lost | Rejected |
| Unified diff (patch format) | Standard | Complex to implement, overkill | Rejected |
| Line-by-line diff with color | Easy to read, simple to implement | Not as precise as unified | **Chosen** |
| Block overwrite entirely | Safe | Too restrictive | Rejected |

## Consequences
- Users can always see what will change before confirming
- `.bak` files provide a safety net
- `confirm_write` now takes a `color` parameter for diff coloring

## References
- @see ADR-014 (tool annotations — write confirmation)
