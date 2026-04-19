# 036: Documentation Coverage

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#100](https://github.com/rkristelijn/llama-cli/issues/100)

## Problem

No way to verify that all public functions/modules have corresponding documentation. Code and docs can drift apart.

## Idea

- Calculate documentation coverage: every public function should link to ≥1 markdown file
- Reverse check: every doc should reference real code
- CI check that fails if coverage drops below threshold

### References

- [ADR-010: Documentation Indexing](../adr/adr-010-documentation-indexing.md)
- [ADR-048: Quality Framework](../adr/adr-048-quality-framework.md) — CMMI 2 check
