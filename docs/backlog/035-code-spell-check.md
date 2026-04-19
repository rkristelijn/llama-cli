# 035: Code Spell Check

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#99](https://github.com/rkristelijn/llama-cli/issues/99)

## Problem

No spell checking for code comments, strings, or documentation. Typos slip through review.

## Idea

Add `cspell` or `codespell` to the CI pipeline and pre-commit hooks. Custom dictionary for project-specific terms.

### References

- [ADR-048: Quality Framework](../adr/adr-048-quality-framework.md) — CMMI 1 quality checks
