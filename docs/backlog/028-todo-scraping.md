# 028: TODO Scraping → TECHDEBT.md

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 1.3 · *Issue*: [#92](https://github.com/rkristelijn/llama-cli/issues/92)

## Problem

`make todo` displays TODOs but doesn't persist them. Tech debt is invisible to sprint planning.

## Idea

Pipe TODO/FIXME scraping output to `TECHDEBT.md`, auto-updated in CI.

### Stacking effect

- Tech debt count metric
- Sprint backlog input
- Debt trend tracking

### References

- [ADR-048 §3.3](../adr/adr-048-quality-framework.md#33-cmmi-1--managed-first-release) — CMMI 1 check 1.3
- [Prompt: 06-todo-scraping](../prompts/06-todo-scraping.md)
