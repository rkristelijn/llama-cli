# ADR-025: Central Help Text

*Status*: Implemented · *Date*: 2026-04-12 · *Context*: Help text appeared in two places: `--help` (CLI) and `/help` (REPL). Keeping them in sync manually is error-prone and violates DRY.

## Problem

- `/help` output was hardcoded as multiple `s.out <<` lines in `repl.cpp`
- `--help` was not yet implemented, but would duplicate the same pattern
- Any change to commands or options requires updating multiple files

## Options Considered

| Option | Pros | Cons |
|--------|------|------|
| Keep inline in each location | Simple | Duplicated, easy to drift |
| Single string in a shared header | One place to edit, zero dependencies | Slightly less flexible |
| Dedicated help module (.cpp/.h) | Testable | Overkill for static strings |

## Decision

A single `src/help.h` header with two `constexpr const char*` constants:

- `help::CLI` — shown by `--help` / `-h` in `main.cpp`
- `help::REPL` — shown by `/help` in `repl.cpp`

Both are plain string literals (no runtime cost, no allocation).

## Rationale

- One file to edit when commands or options change
- `constexpr const char*` — zero overhead, no dependency
- Keeps `repl.cpp` and `main.cpp` lean

## Consequences

- `src/help.h` is the single source of truth for all user-facing help text
- `repl.cpp` includes `help.h` and uses `help::REPL`
- `main.cpp` includes `help.h` and uses `help::CLI` for `--help`/`-h`
- When adding a new command or option, only `help.h` needs updating
