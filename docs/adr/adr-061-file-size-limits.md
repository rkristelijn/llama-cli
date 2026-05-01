---
title: "ADR-061: File Size Limits"
summary: Enforce maximum file sizes to drive modular design and improve testability
status: Implemented
date: 2026-04-24
---

## Status

Implemented

## Context

Coverage is stuck at ~55% because the two largest files are hard to test:

| File | Lines | Coverage | Why |
|------|-------|----------|-----|
| repl.cpp | 1593 | 41% | web_search, ensure_searxng, confirm_exec, strip_exec, handle_response — all only reachable through the full REPL loop |
| tui.h | 811 | 5% | markdown renderer, table renderer, spinner, StreamRenderer — four concerns in one header |

Large files accumulate private functions that can only be tested indirectly. Splitting them into focused modules makes each function directly testable.

## Decision

Enforce file size limits via `make check`:

| File type | Max lines | Rationale |
|-----------|-----------|-----------|
| `*.cpp` (source) | 400 | Forces extraction of concerns into modules |
| `*.h` (header) | 300 | Headers should be interfaces, not implementations |
| `*_test.cpp`, `*_it.cpp` | 600 | Test files need more boilerplate |

A new checker `scripts/lint/check-file-size.sh` scans `src/` and fails if any file exceeds its limit. Existing violations are listed as tech debt with a plan to split.

## Planned splits

### repl.cpp (1593 → ~4 files)

- `src/repl/repl.cpp` — REPL loop, dispatch, send_prompt (~400)
- `src/websearch/websearch.cpp` — web_search, ensure_searxng, url_encode (~200)
- `src/repl/confirm.cpp` — confirm_exec, confirm_write, process_write, process_str_replace (~200)
- `src/repl/display.cpp` — strip_exec_annotations, colorize_ai, handle_response (~200)

### tui.h (811 → ~3 files)

- `src/tui/tui.h` — color helpers, banner, prompt, system_msg, error (~100)
- `src/tui/markdown.h` — render_markdown, render_inline, render_line (~200)
- `src/tui/table.h` — render_table, parse_table_cells, visible_length (~150)
- `src/tui/spinner.h` — Spinner class, StreamRenderer class (~150)

## Consequences

- Every function becomes directly unit-testable → coverage rises naturally
- `make check` prevents new large files from being introduced
- Existing violations must be split before the checker is enforced (grace period: 1 sprint)
