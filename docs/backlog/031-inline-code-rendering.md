# 031: Inline Code Rendering

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#95](https://github.com/rkristelijn/llama-cli/issues/95)

## Problem

Backticks are visible in LLM output instead of being rendered as inline code. Markdown rendering handles headings, bold, lists — but not inline `code`.

## Idea

Extend the TUI markdown renderer to detect backtick-wrapped text and render with a distinct style (e.g. dim, reverse, or different color).

### References

- [ADR-016: TUI Design](../adr/adr-016-tui-design.md)
