# 004: Smart Confirmation Prompt

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#82](https://github.com/rkristelijn/llama-cli/issues/82)

## Problem

When the LLM proposes a command or file write, the confirmation prompt only accepts `y`/`n`. This forces a rigid workflow: reject, retype your feedback, wait for a new proposal.

## Idea

Extend the confirmation prompt to accept richer input:

| Input | Behavior |
|-------|----------|
| `y` | Execute/apply as proposed |
| `n` | Reject, return to prompt |
| `c` | Copy command/content to clipboard, don't execute |
| anything else | Treat as a new prompt with the proposal as context |

### Examples

```text
[proposed: exec] find . -name "*.cpp" -exec grep -l "TODO" {} \;
Execute? [y/n/c] c
📋 Copied to clipboard

[proposed: exec] rm -rf build/
Execute? [y/n/c] y, but only delete build/CMakeCache.txt
[proposed: exec] rm build/CMakeCache.txt
Execute? [y/n/c]

[proposed: write src/main.cpp]
Write? [y/n/c] n, use a switch statement instead of if-else
[proposed: write src/main.cpp]
...
```

### Design considerations

- **Clipboard** — use `pbcopy` (macOS), `xclip`/`xsel` (Linux), detect at runtime
- **Context preservation** — free-text input becomes a follow-up prompt with the rejected proposal still in conversation history
- **Directive prefixes** — `y, but ...` applies the proposal then feeds the amendment; `n, ...` rejects and redirects
- **Backward compatible** — `y`/`n` work exactly as before

### Implementation sketch

1. Read confirmation input as full line, not single char
2. If `y` → execute; if `n` → return; if `c` → pipe to clipboard util
3. Otherwise → inject as new user message, prepend "Regarding your proposed `{action}`: {user_input}"

## References

- [ADR-014: Tool Annotations](../adr/adr-014-tool-annotations.md) — confirmation flow for LLM-proposed actions
- [ADR-015: Command Execution](../adr/adr-015-command-execution.md) — `<exec>` confirmation
- [ADR-019: Safe File Writes](../adr/adr-019-safe-file-writes.md) — write confirmation
