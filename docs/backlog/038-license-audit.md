# 038: License Audit

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#102](https://github.com/rkristelijn/llama-cli/issues/102)

## Problem

No verification that vendored/fetched dependencies have compatible licenses (MIT/BSD/Apache).

## Idea

Script that checks LICENSE files in all FetchContent dependencies and flags incompatible licenses. Run as `make license-check`.
