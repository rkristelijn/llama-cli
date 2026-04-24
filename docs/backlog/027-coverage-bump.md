# 027: Coverage Bump 55% → 60%

*Status*: Done · *Date*: 2026-04-24 · *CMMI*: 1.1 · *Issue*: [#91](https://github.com/rkristelijn/llama-cli/issues/91)

## Problem

CI coverage threshold was 55%. CMMI 1 requires ≥ 60%.

## Solution

Added `ollama_test.cpp` with httplib::Server mock (15 tests, 0 new deps).
Coverage went from 52.7% → 60.6%. Threshold bumped to 60% in `ci.yml`.

See [ADR-058](../adr/adr-058-http-mock-testing.md) for the mock approach.

### References

- [ADR-048 §3.3](../adr/adr-048-quality-framework.md#33-cmmi-1--managed-first-release) — CMMI 1 check 1.1
- [Prompt: 05-coverage-bump](../prompts/05-coverage-bump.md)
