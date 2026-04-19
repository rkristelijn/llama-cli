# 041: Syntax Highlighting via tree-sitter

*Status*: Idea · *Date*: 2026-04-19

## Problem

Code blocks in LLM output are displayed as dimmed plain text. There is no syntax highlighting, making it harder to read generated code — especially for larger snippets.

## Idea

Use [tree-sitter](https://tree-sitter.github.io/) as an optional plugin for syntax highlighting inside fenced code blocks. tree-sitter is a C library with incremental parsing, supporting 100+ languages.

### Why tree-sitter

- Written in C — native integration with our C++ codebase
- Incremental parsing — compatible with streaming responses
- Language grammars are separate `.so` plugins — load only what's needed
- Battle-tested — used by Neovim, Helix, GitHub, and others

### Approach

1. Detect the language tag after ` ``` ` (e.g. ` ```java `, ` ```cpp `)
2. Buffer the code block content
3. Parse with tree-sitter using the matching grammar
4. Map tree-sitter node types (keyword, string, comment, etc.) to ANSI colors
5. Emit highlighted output to terminal

### Considerations

- **Dependency size**: tree-sitter core is small (~100KB), but each language grammar adds a shared library
- **Fallback**: if no grammar is available for a language, fall back to dimmed plain text (current behavior)
- **Distribution**: grammars could be bundled for common languages (C, C++, Python, Java, JavaScript, Rust, Go, Bash) or downloaded on demand
- **Alternative**: md4c (SAX-style markdown parser) could replace the custom StreamRenderer for full CommonMark support, with tree-sitter handling only code blocks

### References

- [tree-sitter](https://github.com/tree-sitter/tree-sitter) — incremental parsing library
- [031: Inline Code Rendering](031-inline-code-rendering.md) — related backlog item
- [ADR-016: TUI Design](../adr/adr-016-tui-design.md)
