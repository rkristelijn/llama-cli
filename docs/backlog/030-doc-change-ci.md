# 030: Doc-Change CI Enforcement

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 1.8 · *Issue*: [#94](https://github.com/rkristelijn/llama-cli/issues/94)

## Problem

No CI check ensures docs are updated when source code changes. API/UI changes can ship without documentation.

## Idea

CI job that fails if `src/` is modified but `docs/` is not touched in the same PR.

### References

- [ADR-048 §3.3](../adr/adr-048-quality-framework.md#33-cmmi-1--managed-first-release) — CMMI 1 check 1.8
- [Prompt: 08-doc-change-ci](../prompts/08-doc-change-ci.md)
