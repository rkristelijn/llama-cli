# 032: Mermaid Diagram Rendering

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#96](https://github.com/rkristelijn/llama-cli/issues/96)

## Problem

LLM outputs Mermaid diagrams in fenced code blocks but they render as raw text in the terminal.

## Idea

Detect ` ```mermaid ` blocks and render as ASCII art or pass to an external renderer (e.g. mermaid-tui, mermaid-cli → sixel/kitty graphics).

### References

- [ADR-016: TUI Design](../adr/adr-016-tui-design.md)
