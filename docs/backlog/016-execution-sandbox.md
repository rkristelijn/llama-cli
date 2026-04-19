# 016: Execution Sandbox

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#55](https://github.com/rkristelijn/llama-cli/issues/55)

## Problem

The exec module allows direct shell execution, which is unsafe for autonomous agent workflows.

## Idea

- Command whitelist/blacklist (see also [010](010-command-permissions.md))
- Max execution depth and timeout limits
- Dry-run mode — show what would execute without running
- Execution approval hook (interactive or policy-based)
- All executed commands logged with metadata

### Benefits

- Prevents destructive AI behavior
- Safe for autonomous use
- Required for multi-agent setups

### References

- [ADR-028: Execution Limits](../adr/adr-028-execution-limits.md)
- [010 — Command permissions](010-command-permissions.md)
- [015 — Planner/executor](015-planner-executor.md)
