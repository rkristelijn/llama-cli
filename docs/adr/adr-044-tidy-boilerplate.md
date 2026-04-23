# ADR-044: Tidy Build Boilerplate

*Status*: Accepted · *Date*: 2026-04-19

## Context

The project's build automation had grown organically, leading to redundant logic, inconsistent naming, and a confusing relationship between the `Makefile` and shell scripts.

## Decision

We have established a clear **Orchestration Layer** pattern using the `Makefile` and organized scripts.

### 1. Makefile as Orchestrator

The `Makefile` serves as the primary entry point for all developer tasks. It defines high-level "aggregators" that manage dependencies and call focused sub-targets.

| Target | Description | Aggregates |
|--------|-------------|------------|
| `make format` | **Auto-fix everything**. | `format-code`, `format-md`, `format-yaml`, `format-scripts` |
| `make lint` | **Passive quality checks**. | `lint-code`, `lint-md`, `lint-yaml`, `lint-makefile`, `lint-scripts`, `tidy`, `complexity` |
| `make test` | **Run all tests**. | `test-unit`, `e2e` |
| `make check` | **Full gate**. | `lint`, `test`, `sast` |

### 2. Scripts as Executors

Execution logic is extracted into focused shell scripts in subdirectories of `scripts/`. Scripts do not call other `make` targets (to avoid recursion and hidden dependencies).

```text
scripts/
├── fmt/            # Active formatters (make format-*)
├── lint/           # Passive linters (make lint-*)
├── test/           # Test runners (make test-*)
├── dev/            # Local helpers (setup, log, todo)
├── gh/             # GitHub CLI helpers
├── git/            # Git hook templates
└── ci/             # CI-specific scripts
```

### 3. Naming Conventions

- **Makefile targets**: kebab-case, organized by `##@` sections.
- **Shell scripts**: kebab-case, verb-object pattern (e.g., `run-unit.sh`, `format-md.sh`).
- **Git hooks**: thin wrappers calling standardized `make` targets.

## Consequences

- **Simplicity**: Developers use a few intuitive commands (`make format`, `make lint`, `make check`).
- **Transparency**: `make help` is auto-generated and clearly categorized.
- **Maintainability**: Tool-specific complexity is isolated in scripts, while the project-wide workflow is visible in the `Makefile`.
- **Consistency**: Local development hooks run the same targets as the CI pipeline.

## Addendum (2026-04-23): Leaf Target Rule

Leaf targets (e.g., `test-unit`, `e2e`, `tidy`, `complexity`) must not depend on
`all` or `build`. They do exactly one thing: run their tool.

Aggregators (`test`, `check`, `quick`) own the build dependency.

```text
# GOOD — leaf does one thing, aggregator orchestrates
test: build test-unit e2e
test-unit:                    # ← no dependency on build
    @bash scripts/test/run-unit.sh

# BAD — hidden build inside a leaf
test-unit: all                # ← surprise cmake + cp on every test run
    @bash scripts/test/run-unit.sh
```

Why this matters:

- `make test-unit` after a manual `cmake --build` should just run tests, not rebuild
- Hidden dependencies make failures hard to trace (a build error looks like a test error)
- Aggregators are the single place where ordering is defined — no implicit chains
