---
title: "ADR-060: Unified Error Output Stream"
summary: Route all error and trace output through an injectable ostream to enable consistent testing and eliminate stderr noise
status: accepted
date: 2026-04-24
---

## Status

Proposed

## Context

Error output currently uses three different mechanisms:

| Source | Mechanism | Capturable in tests? |
|--------|-----------|---------------------|
| `tui::error()` | `std::cerr <<` | Yes, via `rdbuf()` redirect |
| `StderrTrace::log()` | `fprintf(stderr, ...)` | No, needs `dup2()` |
| `LOG_EVENT()` | file-based logger | N/A (separate concern) |

This causes two problems:

1. **Noisy test output** — unit tests for error paths (connection failure, HTTP 404/500) print error messages to the terminal, making test runs look like failures even when all tests pass.
2. **Inconsistent capture** — C++ `std::cerr` and C `fprintf(stderr)` require different redirect mechanisms, so there is no single way to capture all error output in tests.

## Decision

Introduce an injectable `std::ostream& err` for all error output:

1. **`tui::error()`** — already takes an `ostream&`, no change needed.
2. **Ollama functions** — replace hardcoded `std::cerr` with an `ostream&` parameter (or add `err_stream` to `Config`).
3. **`StderrTrace`** — rewrite to use `ostream&` instead of `fprintf(stderr)`.
4. **CI checker** — add a grep-based check to `make check` that flags direct `std::cerr` or `fprintf(stderr` usage outside of the designated error stream wiring. This prevents regressions.

## Consequences

- All error output becomes testable with a simple `std::ostringstream`
- Tests run silently — no misleading error messages in terminal
- One mechanism to capture everything: `ostream& err`
- CI catches accidental direct stderr usage
