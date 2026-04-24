# 042: Unified Error Output Stream

*Status*: Planned · *Date*: 2026-04-24 · *ADR*: [060](../adr/adr-060-unified-error-output.md)

## Problem

Error output uses `std::cerr` (C++) and `fprintf(stderr)` (C) interchangeably. Tests cannot capture both with one mechanism, causing noisy test output that looks like failures.

## Tasks

1. Add `ostream& err` to ollama functions (replace hardcoded `std::cerr`)
2. Rewrite `StderrTrace::log()` to use `ostream&` instead of `fprintf(stderr)`
3. Update all tests to inject `std::ostringstream` and assert error messages
4. Add CI checker: `scripts/check/check-stderr.sh` — grep for direct `std::cerr` and `fprintf(stderr` in `.cpp` files, whitelist only the wiring points
5. Wire checker into `make check`

## Acceptance

- `make test` produces zero output on success (only doctest summary)
- `make check` fails if new code writes directly to stderr
- All error messages are asserted in their respective tests
