# 034: Fix Release Pipeline

*Status*: ✅ Done · *Date*: 2026-04-19 · *Issue*: [#98](https://github.com/rkristelijn/llama-cli/issues/98)

## Problem

Release is broken: version shows just `v`, download filenames are wrong. Users can't easily download the tool.

## Solution (implemented)

- Auto-versioning from conventional commits (patch/minor/major detection)
- Multi-platform builds: linux x64/arm64, macOS arm64
- Correct binary naming (`llama-cli-linux-x64`, etc.)
- Changelog generation via git-cliff
- GitHub Release with artifacts and checksums
- One-liner install script in README

### References

- [ADR-045: Fix Release Pipeline](../adr/adr-045-fix-release-pipeline.md)
