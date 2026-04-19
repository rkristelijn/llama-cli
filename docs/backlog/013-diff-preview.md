# 013: Diff Preview Before Overwrite

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#50](https://github.com/rkristelijn/llama-cli/issues/50)

## Problem

When the LLM proposes writing to an existing file, the user can't see what will change. The entire file gets replaced, which caused real data loss (TODO.md overwritten).

## Idea

- If file exists: show unified diff (old vs new) before asking `y/n`
- Add `d` option: `[y/n/s/d]` where `d` = show diff
- Auto-backup to `.bak` before overwriting
- New files: current behavior is fine

### References

- [ADR-019: Safe File Writes](../adr/adr-019-safe-file-writes.md)
- [004 — Smart confirmation](004-smart-confirmation.md)
