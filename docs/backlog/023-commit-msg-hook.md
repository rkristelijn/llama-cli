# 023: Commit Message Validation

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 0.1 · *Issue*: [#87](https://github.com/rkristelijn/llama-cli/issues/87)

## Problem

No enforcement of conventional commit format. Blocks auto-changelog and release note generation.

## Idea

Add `commit-msg` git hook checking conventional commit format (`feat:`, `fix:`, `chore:`, etc.).

### Stacking effect

- Auto-changelog generation
- Release note generation
- CI routing by commit type

### References

- [ADR-048 §3.2](../adr/adr-048-quality-framework.md#32-cmmi-0--essentials-mvp) — CMMI 0 check 0.1
- [Prompt: 01-commit-msg-hook](../prompts/01-commit-msg-hook.md)
