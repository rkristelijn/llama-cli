# 009: Package Distribution

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#39](https://github.com/rkristelijn/llama-cli/issues/39)

## Problem

Users must build from source or use the install script. No native package manager support.

## Idea

- **Homebrew**: create a tap (`brew install rkristelijn/tap/llama-cli`)
- **apt**: `.deb` package or PPA
- **Static binary**: AppImage or fully static Linux build
- CI builds release artifacts for all targets automatically

### References

- [ADR-045: Fix Release Pipeline](../adr/adr-045-fix-release-pipeline.md)
