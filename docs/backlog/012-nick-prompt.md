# 012: Nick / Custom Prompt

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#49](https://github.com/rkristelijn/llama-cli/issues/49)

## Problem

When running multiple terminals with different contexts, all prompts look the same (`>`).

## Idea

- `/nick qa` → prompt becomes `qa>`
- `/nick dev` → `dev>`
- `/nick` (no args) → reset to default `>`

Useful for distinguishing sessions visually.
