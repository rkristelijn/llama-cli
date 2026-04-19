# 040: CI Check Caching

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#104](https://github.com/rkristelijn/llama-cli/issues/104)

## Problem

All CI checks run on every push even when their inputs haven't changed. Wastes time and compute.

## Idea

Cache check results keyed by input file hashes. Skip checks when inputs are unchanged. Applies to: lint, SAST, complexity, comment ratio.
