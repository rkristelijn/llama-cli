# ADR-052: Markdown Renderer

## Status

Implemented

Implemented

## Date

2026-04-19

## Context

LLM responses are markdown-heavy: headings, bold, code blocks, tables, lists, links. The original renderer (ADR-016) handled only basic formatting. Users reported that inline code, commands, and tables were not rendered properly. With streaming now implemented, the renderer must also work token-by-token without seeing the full response upfront.

We need a renderer that:

- Covers the most common markdown elements in LLM output
- Works in both batch mode (full response) and streaming mode (token-by-token)
- Adapts table layout to terminal width
- Degrades gracefully when color is disabled

## Decision

### Supported elements

| Element | Syntax | ANSI rendering |
|---------|--------|----------------|
| Heading | `# text` | bold + underline |
| Bold | `**text**` | bold (`\033[1m`) |
| Italic | `*text*` | italic (`\033[3m`) |
| Bold+italic | `***text***` | bold+italic (`\033[1;3m`) |
| Inline code | `` `text` `` | cyan (`\033[36m`) |
| Strikethrough | `~~text~~` | strikethrough (`\033[9m`) |
| Link | `[text](url)` | text underlined, url dimmed |
| Code block | ` ``` ` fences | dim (`\033[2m`) |
| Bullet list | `- item` / `* item` | `•` prefix, 2-space indent |
| Ordered list | `1. item` | preserved number, 2-space indent |
| Nested list | `- sub` | additional indent per nesting level |
| Blockquote | `> text` | dim `│` prefix |
| Horizontal rule | `---` / `***` / `___` | dim `────────────────────` |
| Table | `\| col \| col \|` | terminal-width-aware column padding |

### Inline rendering order

The char-by-char parser tries matchers in this order to avoid ambiguity:

1. `***` bold+italic (before `**` and `*`)
2. `**` bold
3. `*` italic
4. `~~` strikethrough
5. `` ` `` inline code
6. `[` link

### Table rendering

Tables are the most complex element because they require seeing all rows before rendering:

1. Detect table rows (lines starting with `|`)
2. Buffer consecutive table rows
3. On first non-table line (or end of input), flush the buffer:
   - Parse cells, count columns, measure max content width per column
   - Query terminal width via `ioctl(STDOUT_FILENO, TIOCGWINSZ)`
   - Distribute available width proportionally across columns
   - Render separator rows as dim dashed lines
   - Pad data cells to computed column width
4. Fall back to 80 columns when not a TTY

### Streaming integration

`StreamRenderer` processes tokens character-by-character, buffering until a newline arrives. Block-level elements (tables, code fences) require cross-line state:

- **Code blocks**: `in_code_block_` flag toggled on ` ``` ` fences
- **Tables**: `table_buf_` collects rows until a non-table line triggers flush
- **`finish()`**: flushes any pending table buffer and incomplete line

### Color-off behavior

When color is disabled (`--no-color`, `NO_COLOR` env, non-TTY):

- `render_markdown()` returns the input unchanged (zero processing)
- `StreamRenderer` still buffers tables for alignment, but without ANSI codes

## Consequences

- All markdown rendering lives in `src/tui/tui.h` (inline functions + `StreamRenderer` class)
- Tests in `src/tui/markdown_it.cpp` cover all elements in both batch and streaming mode
- Adding a new inline element = add a `try_*` function + insert it in `render_inline` at the right priority
- Adding a new block element = handle in `render_line` + mirror in `StreamRenderer::flush_line`
- Terminal width query is Linux/macOS only (`sys/ioctl.h`) — acceptable for a CLI tool

## References

- @see ADR-016 (TUI design — color scheme, API)
- @see `src/tui/tui.h` (implementation)
- @see `src/tui/markdown_it.cpp` (tests)
- @see [docs/backlog/031-inline-code-rendering.md](../backlog/031-inline-code-rendering.md)
- @see [docs/backlog/041-tree-sitter-highlighting.md](../backlog/041-tree-sitter-highlighting.md)
