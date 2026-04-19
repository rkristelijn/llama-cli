# 039: Local CI Testing

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#103](https://github.com/rkristelijn/llama-cli/issues/103)

## Problem

CI YAML issues are only caught after pushing. No way to test the pipeline locally.

## Idea

Run CI pipeline locally via [`act`](https://github.com/nektos/act) or Docker to catch YAML/workflow issues before pushing.
