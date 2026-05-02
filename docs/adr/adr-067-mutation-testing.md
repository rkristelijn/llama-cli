# ADR-067: Mutation Testing via Mull

*Status*: Proposed

## Context

Unit tests verify that code works. Mutation testing verifies that *tests* work —
by injecting small faults ("mutants") into the source and checking that at least
one test fails. A surviving mutant means a gap in test coverage that line-coverage
metrics miss.

This is the strongest defense against "AI-slop": an LLM-generated test that
doesn't actually validate logic will be exposed by surviving mutants.

## Decision

Add **Mull** (LLVM-based mutation testing) as an optional PR-level quality gate.

### Why Mull

| Criterion | Mull | Alternatives |
|-----------|------|--------------|
| Works on LLVM IR | ✅ | Custom scripts: fragile |
| No source modification | ✅ (bitcode-level) | sed-based: risky |
| Git-diff mode | ✅ (only mutate changed files) | Most tools: full repo |
| C++17 support | ✅ | dextool: limited |
| Active maintenance | ✅ (Trail of Bits) | llvm-mutator: experimental |

### Integration point

Mutation testing runs **only in CI on pull requests**, as the final check after
all other gates pass. It is never part of `make quick` or `make check`.

```text
PR pipeline order:
  1. make check          (lint, tidy, tests, sast)
  2. make mutation       (Mull on changed files only — slow but targeted)
```

### Makefile target

```makefile
mutation: ## Run mutation testing on changed files (PR only)
	@echo "==> mutation testing (mull)..."
	@bash scripts/test/run-mutation.sh
```

### Script design

See `scripts/test/run-mutation.sh` for the implementation. Key behavior:

- On Linux (CI): runs Mull natively against test binaries
- On macOS: skips (Apple libc++ incompatible with Mull's LLVM IR instrumentation)
- Targets: `test_config`, `test_json`, `test_annotation`, `test_exec`
- Config: `.config/mull.yml` (arithmetic, comparison, boundary, logical mutators)

### Thresholds

Start without a hard gate — report-only mode:

| Phase | Mutation score | Action |
|-------|---------------|--------|
| Phase 1 (now) | Report only | Visibility, no blocking |
| Phase 2 | > 40% | Warn on PR if below |
| Phase 3 | > 60% | Block PR if below |

### What it costs

- **Time**: ~2-5 min per PR (only changed files are mutated)
- **Dependencies**: `mull` (installable via apt/brew, LLVM-based)
- **CI resources**: Runs after all other checks pass (fail-fast: if tests fail, mutation is skipped)

## Consequences

- Surviving mutants reveal tests that pass by coincidence, not by logic
- Forces meaningful assertions (not just "it doesn't crash")
- Catches AI-generated tests that look good but validate nothing
- Does NOT replace coverage — complements it (coverage = breadth, mutation = depth)

## Not in scope

- Running on every commit (too slow)
- Running locally in `make check` (developer friction)
- Mutating test files themselves
