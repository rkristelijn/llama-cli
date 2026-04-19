# 033: Tab Autocompletion

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#97](https://github.com/rkristelijn/llama-cli/issues/97)

## Problem

No tab completion for file paths. Users must type full paths when referencing files.

## Idea

When input starts with `~`, `.`, or `/`, tab-complete file paths using linenoise hints or a custom completer.

### References

- [ADR-012: Interactive REPL](../adr/adr-012-interactive-repl.md)
