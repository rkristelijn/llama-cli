# ADR-086: Syntax Highlighting via tree-sitter

*Status*: Proposed · *Date*: 2026-05-05

## Context

Code blocks in LLM output use regex-based highlighting (ADR-052). This works for common patterns but lacks accuracy for complex syntax. tree-sitter provides incremental parsing with 100+ language grammars.

## Decision

Use [tree-sitter](https://tree-sitter.github.io/) as an optional enhancement for syntax highlighting inside fenced code blocks.

### Approach

1. Detect language tag after ` ``` `
2. Buffer code block content
3. Parse with tree-sitter using matching grammar
4. Map node types (keyword, string, comment) to theme colors
5. Fall back to current regex highlighter if no grammar available

### Why tree-sitter

- Written in C — native integration with C++ codebase
- Incremental parsing — compatible with streaming responses
- Language grammars are separate `.so` plugins — load only what's needed
- Battle-tested (Neovim, Helix, GitHub)

## Consequences

- Accurate highlighting for all supported languages
- Additional dependency (~100KB core + grammars)
- Must bundle or download grammars for common languages
- Fallback to regex highlighter keeps current behavior as baseline

## References

- [ADR-016: TUI Design](adr-016-tui-design.md)
- [ADR-052: Markdown Renderer](adr-052-markdown-renderer.md)
- [tree-sitter](https://github.com/tree-sitter/tree-sitter)
