# 034: Fix Release Pipeline

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#98](https://github.com/rkristelijn/llama-cli/issues/98)

## Problem

Release is broken: version shows just `v`, download filenames are wrong. Users can't easily download the tool.

## Idea

- Fix VERSION file / tag extraction in release workflow
- Correct binary artifact naming in CI
- Update README install section with working download links

### References

- [ADR-045: Fix Release Pipeline](../adr/adr-045-fix-release-pipeline.md)
