# ADR-017: Integration Tests

## Status

Accepted

Accepted

## Date

2026-04-11

## Context

Unit tests cover individual functions, but we lack tests that verify complete user flows end-to-end. We need integration tests that simulate a full REPL session with multiple commands, verifying that features work together correctly.

## Decision

### Approach

- Integration tests use the same `run_repl(ChatFn, Config, istream, ostream)` injection as unit tests
- A smart mock `ChatFn` simulates LLM behavior: echoes, annotations, exec proposals
- Tests are documented with Gherkin `.feature` files as living specifications
- Test implementation uses doctest SCENARIO/GIVEN/WHEN/THEN (already Gherkin-compatible)

### Test structure

```text
test/
  test_config.cpp            # unit test (_test convention)
  test_conversation_it.cpp   # integration test (_it convention)
  test_helpers.h             # shared MockLLM + test_cfg
```

### Naming convention

- `*_test.cpp` — unit test: tests one module in isolation
- `*_it.cpp` — integration test: tests a feature flow end-to-end
- Prepares for feature module decomposition where each module contains its own `_test.cpp` and `_it.cpp`

### What is tested

- Full conversation flow with history
- Command chaining: chat → !! → chat about output
- /set toggles persist across prompts
- /version, /clear, /help
- Write annotations with y/n/s confirmation
- Exec annotations with y/n confirmation
- Unknown commands

### What is NOT tested (requires HTTP)

- `ollama_generate()` and `ollama_chat()` — these need a mock HTTP server (future)

## Alternatives considered

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| Cucumber/Gherkin runner | Real BDD, auto-links spec to code | Heavy dependency, C++ support poor | Rejected |
| `.feature` + doctest | Spec as docs, tests in familiar framework | Manual sync between spec and test | **Chosen** |
| No spec, just more unit tests | Simple | Hard to see full flows, no living docs | Rejected |

## Consequences

- `.feature` files serve as living documentation for user-facing behavior
- Integration tests catch regressions in feature interactions
- Mock ChatFn can be extended to simulate more complex LLM behaviors

## References

- @see ADR-008 (test framework — doctest, GWT style)
- @see ADR-012 (REPL — injectable I/O)
