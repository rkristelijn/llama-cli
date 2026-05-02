# ADR-071: Markdown Module Split

*Status*: Implemented

## Date

2026-05-02

## Context

`src/tui/markdown.cpp` has grown to handle both block-level rendering (batch mode) and streaming rendering (token-by-token `StreamRenderer` class). At ~350 lines it approaches the file size limit (ADR-061) and mixes two distinct responsibilities:

1. **Block rendering** — stateless functions that transform complete markdown lines to ANSI
2. **Stream rendering** — stateful `StreamRenderer` class that buffers tokens and renders on newline

These have different change patterns: block rendering changes when new markdown elements are added, stream rendering changes when buffering/annotation logic changes.

## Decision

Split `markdown.cpp` into three files:

| File | Responsibility | Lines (est.) |
|------|---------------|-------------|
| `src/tui/markdown.cpp` | Inline + block rendering functions (render_line, render_inline, try_*) | ~200 |
| `src/tui/markdown_stream.cpp` | StreamRenderer class implementation | ~150 |
| `src/tui/markdown.h` | Public interface (unchanged) | ~60 |

### Split boundary

- `markdown.cpp` keeps: `namespace tui { ... }` — all free functions
- `markdown_stream.cpp` gets: `StreamRenderer::*` methods (write, finish, flush_line, etc.)
- Both include `markdown.h` — no circular dependencies

### Build impact

- `CMakeLists.txt`: add `src/tui/markdown_stream.cpp` to `LIB_SOURCES` and `REPL_LIBS`
- No API changes — `markdown.h` stays the same
- Tests continue to work unchanged

## Consequences

- Each file has a single responsibility
- Easier to review changes to streaming vs. block rendering independently
- Both files stay well under the 300-line limit
- No functional changes — pure refactor
