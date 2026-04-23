# ADR-055: Layered Test Strategy — From Unit Tests to Fuzzing

*Status*: Accepted · *Date*: 2026-04-23 · *Extends*: [ADR-048](adr-048-quality-framework.md) (CMMI 0→1), [ADR-017](adr-017-integration-tests.md), [ADR-008](adr-008-test-framework.md)

## Context

The project had unit tests and e2e tests but lacked:

- **Edge case discovery** — tests only cover cases a human thought of
- **Feature traceability** — no way to see which features are tested and which aren't
- **Input robustness** — annotation parsers process untrusted LLM output but were never tested with adversarial input

CodeRabbit review on PR #107 found three logic bugs (stale history entry on interrupt, unbounded followup loop, silent write failure) that no existing test or linter caught. These are the class of bugs that require a higher level of testing.

## Decision

Adopt a four-layer test strategy, each layer catching what the previous misses:

| Layer | What it catches | Tool | When to run |
|-------|----------------|------|-------------|
| **Unit tests** (SCENARIO) | Logic bugs in known paths | doctest | `make test` / pre-push |
| **Integration tests** | Cross-module interaction | doctest (run_repl) | `make test` / pre-push |
| **Fuzzing** | Crashes, hangs, UB on unexpected input | libFuzzer + ASan/UBSan | `make fuzz` / periodic |
| **Feature spec** | Missing test coverage for features | `make features` | pre-push / PR review |

### Layer 1–2: Unit + Integration (existing)

BDD-style scenarios using doctest `SCENARIO`/`GIVEN`/`WHEN`/`THEN`. Each scenario name **is** the feature specification. Convention: **no feature without a SCENARIO**.

```bash
make features    # list all 48 tested scenarios
make test        # run all unit + e2e tests
```

### Layer 3: Fuzzing (new)

libFuzzer targets for code that processes untrusted input (LLM responses). Compiled with ASan + UBSan to catch memory errors and undefined behavior.

```bash
make fuzz        # 60-second annotation parser fuzz run
```

Current fuzz targets:

- `fuzz_annotation` — exercises `parse_write_annotations`, `parse_str_replace_annotations`, `parse_read_annotations`, `strip_annotations` with random input

Future fuzz targets (when coverage warrants it):

- `fuzz_json` — JSON extraction from arbitrary strings
- `fuzz_markdown` — markdown renderer with adversarial input

### Layer 4: Feature spec (new)

`make features` lists all SCENARIO names from all test binaries. This is the living feature specification. During PR review, check: does every new feature have a corresponding SCENARIO?

### Future: Mutation testing (CMMI 2)

Per [ADR-048 §3.4](adr-048-quality-framework.md), mutation testing (mull) is planned for CMMI 2 when coverage exceeds 70%. It answers: "are our tests actually testing the right thing?"

## Consequences

- **CI runs layers 1–2 only** (unit + integration + e2e) — these are fast and deterministic
- **Fuzzing is not in CI** — it requires clang with libFuzzer (not available on all runners), runs are non-deterministic, and a 60-second fuzz run would slow every PR. Run locally with `make fuzz` before releases or after parser changes.
- **`make features` is not a CI gate** — it's a review aid. Run locally or add as PR comment artifact if desired.
- New convention: every feature needs a SCENARIO, reviewable via `make features`
- Fuzz corpus grows over time in `.tmp/fuzz-corpus/` (gitignored)

## Cross-references

- [ADR-048](adr-048-quality-framework.md) — Quality framework, CMMI levels, automation stacking
- [ADR-008](adr-008-test-framework.md) — doctest as test framework
- [ADR-017](adr-017-integration-tests.md) — Integration test patterns (run_repl with mock chat)
- [ADR-029](adr-029-repl-e2e.md) — REPL e2e testing
- [ADR-032](adr-032-e2e-test-improvements.md) — E2E test improvements
