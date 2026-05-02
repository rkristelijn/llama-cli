# 033: Tab Autocompletion

*Status*: ✅ Done · *Date*: 2026-04-19 · *Issue*: [#97](https://github.com/rkristelijn/llama-cli/issues/97)

## Problem

No tab completion for file paths. Users must type full paths when referencing files.

## Solution (implemented)

Tab-completion for slash commands and file paths via linenoise:

- `/` commands auto-complete (e.g. `/he` → `/help`)
- File paths complete after `~`, `.`, `/`, or within commands that take paths
- Implementation in `repl.cpp`: `slash_completion()` and `path_completions()`

### References

- [ADR-012: Interactive REPL](../adr/adr-012-interactive-repl.md)
