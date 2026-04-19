# 037: Dependency Outdated Check

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#101](https://github.com/rkristelijn/llama-cli/issues/101)

## Problem

C++ deps are pinned via FetchContent `GIT_TAG` in CMakeLists.txt. No automated way to check if newer versions exist.

## Idea

Script that compares pinned GIT_TAG values against latest GitHub releases for each dependency. Run in CI or as `make dep-check`.

### References

- [ADR-026: Version Pinning](../adr/adr-026-version-pinning.md)
