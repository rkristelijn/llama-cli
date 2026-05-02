# 004: Smart Confirmation Prompt

*Status*: ✅ Done · *Date*: 2026-04-19 · *Issue*: [#82](https://github.com/rkristelijn/llama-cli/issues/82)

## Problem

When the LLM proposes a command or file write, the confirmation prompt only accepts `y`/`n`. This forces a rigid workflow: reject, retype your feedback, wait for a new proposal.

## Solution (implemented)

Confirmation prompts now accept rich input:

| Input | Write | Exec | Behavior |
|-------|-------|------|----------|
| `y` | ✅ | ✅ | Execute/apply as proposed |
| `n` | ✅ | ✅ | Reject, return to prompt |
| `c` | ✅ | ✅ | Copy content/command to clipboard (pbcopy/xclip) |
| `s` | ✅ | — | Show full content again |
| `t` | ✅ | ✅ | Trust all — auto-approve remaining actions this session |

Also implemented: `--auto-confirm-write` for automated workflows.

### Not implemented (future)

- Free-text input as follow-up prompt (e.g. "n, use a switch statement instead") — currently only single-char options are recognized; anything else re-prompts.

## References

- [ADR-014: Tool Annotations](../adr/adr-014-tool-annotations.md) — confirmation flow for LLM-proposed actions
- [ADR-015: Command Execution](../adr/adr-015-command-execution.md) — `<exec>` confirmation
- [ADR-019: Safe File Writes](../adr/adr-019-safe-file-writes.md) — write confirmation
