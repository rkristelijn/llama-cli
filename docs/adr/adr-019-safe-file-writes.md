# ADR-019: Safe File Writes — Auto-Diff, Backup, and Targeted Edits

## Status
Accepted (updated 2026-04-16)

## Date
2026-04-11 (original), 2026-04-16 (updated)

## Context
The LLM can propose file writes via `<write>` and targeted edits via `<str_replace>`. When overwriting an existing file, the user needs to see what will change before confirming.

## Decision

### Auto-diff preview
- Existing files: diff is shown **automatically** before the prompt (no `d` option needed)
- New files: full content is shown before the prompt
- Diff uses Myers LCS algorithm (via `dtl` library) for accurate change detection

### Targeted edits with `<str_replace>`
- Preferred over full-file `<write>` for existing files — safer, less error-prone
- Shows diff of only the changed section (old → new)
- Prompt: `Apply str_replace to path? [y/n]`
- Validates that old_str exists exactly once in the file

### Automatic backup
- Before overwriting an existing file, a `.bak` copy is created
- Backup is silent — no confirmation needed

### Diff format
LCS-based diff with git-style prefixes:
```diff
  unchanged line
- removed line      (red)
+ added line        (green)
```

### Confirmation prompts

| Action | Prompt |
|--------|--------|
| `<write>` | `Write to path? [y/n/s]` |
| `<str_replace>` | `Apply str_replace to path? [y/n]` |

## Consequences
- Users always see what will change before confirming
- `.bak` files provide a safety net
- `<str_replace>` reduces risk of accidental full-file overwrites
- `dtl` library added as header-only dependency via FetchContent

## References
- @see ADR-014 (tool annotations)
