# 024: Branch Naming Validation

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 0.2 · *Issue*: [#88](https://github.com/rkristelijn/llama-cli/issues/88)

## Problem

`pre-commit.sh` blocks commits to `main` but doesn't validate branch name format (e.g. `feat/123-short-name`).

## Idea

Add regex check to pre-commit: `^(feat|fix|chore|spike|release)/[0-9]+-[a-z0-9-]+$`

### Stacking effect

- Issue linking via branch name
- CI trigger routing
- Traceability (commit → branch → issue → ADR)

### References

- [ADR-048 §3.2](../adr/adr-048-quality-framework.md#32-cmmi-0--essentials-mvp) — CMMI 0 check 0.2
- [Prompt: 02-branch-naming](../prompts/02-branch-naming.md)
