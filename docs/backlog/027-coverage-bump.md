# 027: Coverage Bump 55% → 60%

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 1.1 · *Issue*: [#91](https://github.com/rkristelijn/llama-cli/issues/91)

## Problem

CI coverage threshold is 55%. CMMI 1 requires ≥ 60%.

## Idea

Add tests to close the gap, then bump the threshold in `ci.yml`.

### References

- [ADR-048 §3.3](../adr/adr-048-quality-framework.md#33-cmmi-1--managed-first-release) — CMMI 1 check 1.1
- [Prompt: 05-coverage-bump](../prompts/05-coverage-bump.md)
