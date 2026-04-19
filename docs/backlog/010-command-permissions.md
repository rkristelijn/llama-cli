# 010: Command Permissions

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#42](https://github.com/rkristelijn/llama-cli/issues/42)

## Problem

All LLM-proposed commands require the same `y/n` confirmation. Read-only commands are safe; destructive commands should be blocked entirely.

## Idea

Three permission levels:

| Level | Commands | Behavior |
|-------|----------|----------|
| **allow** | `cat`, `ls`, `tree`, `head`, `tail`, `wc`, `grep`, `find`, `stat`, `du`, `df` | Auto-execute |
| **confirm** | `<write>`, `mv`, `cp`, `chmod`, `sed -i` | Ask y/n |
| **block** | `rm`, `rmdir`, `mkfs`, `dd`, `>` (truncate) | Never execute |

### Safety rules

- Pipe chains evaluated by most destructive command
- `sudo` always requires confirmation
- `git push`, `git reset --hard` require confirmation
- `/set permissions` to view; `/allow`, `/block` for per-session overrides

### References

- [ADR-015: Command Execution](../adr/adr-015-command-execution.md)
- [ADR-028: Execution Limits](../adr/adr-028-execution-limits.md)
