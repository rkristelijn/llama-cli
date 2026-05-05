# ADR-075: Test Best Practices — Lessons Learned

*Status*: Accepted · *Date*: 2026-05-05 · *Extends*: [ADR-055](adr-055-layered-test-strategy.md), [ADR-008](adr-008-test-framework.md)

## Context

Repeated CI failures revealed patterns of fragile tests that pass locally but fail in CI (or vice versa). This ADR codifies lessons learned into enforceable rules.

## Decision

### Rule 1: No timing-dependent assertions

**Problem**: `sleep N` with timeout `M < N` relies on the scheduler. Coverage builds add instrumentation overhead that changes timing.

**Fix**: Use commands that produce output so timeout checks trigger in the read loop.

```cpp
// ❌ Bad — fgets blocks until sleep finishes, timeout never triggers
auto r = cmd_exec("sleep 2", 1, 1000);
CHECK(r.timed_out);

// ✅ Good — output triggers timeout check in the loop
auto r = cmd_exec("for i in $(seq 1 100); do echo x; sleep 0.1; done", 1, 1000);
CHECK(r.timed_out);
```

### Rule 2: Test the contract, not the implementation

**Problem**: `CHECK(r.output.empty())` assumes timeout means no output. But a command can produce output *before* timing out.

**Fix**: Assert on the observable contract (timeout marker present), not on side-effects.

```cpp
// ❌ Bad — assumes no output before timeout
CHECK(r.output.empty());

// ✅ Good — asserts the documented behavior
CHECK(r.output.find("[killed: timeout") != std::string::npos);
```

### Rule 3: Handle type coercion explicitly

**Problem**: `json_extract_string(json, "num")` returned `""` for `{"num":42}` because it only handled quoted strings. Tests assumed cross-type extraction would work.

**Fix**: Functions that extract values should document and handle type boundaries. If a numeric value is requested as string, return the string representation.

### Rule 4: Mark known flaky tests with TODO

When a test is inherently timing-sensitive and cannot be made deterministic, annotate it:

```cpp
// TODO: flaky on coverage builds — timeout race with instrumentation overhead
```

This makes flakiness visible in `make todo` output and prevents wasted debugging time.

### Rule 5: CI-reproducible tests

Every test must pass in these environments:

- Local macOS (developer machine)
- CI Ubuntu 24.04 (GitHub Actions)
- Coverage build (--coverage flags, ~2x slower)
- Sanitizer build (ASan/UBSan, ~3x slower)

If a test cannot be made reliable across all four, gate it behind a condition or move it to `make live`.

## Consequences

- Fewer CI-only failures
- Tests document behavior, not implementation details
- `make todo` surfaces known fragile tests
- New tests are reviewed against these rules in PR review
