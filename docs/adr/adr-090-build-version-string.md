# ADR-090: Build Version String

*Status*: Accepted · *Date*: 2026-05-06

## Context

We need to distinguish local dev builds from CI release builds at a glance.

## Decision

The `make build` output shows a version string with two variants:

| Context | Format | Example |
|---------|--------|---------|
| Local (dirty) | `✓ a3f7b2c dirty` | uncommitted .cpp/.h changes |
| Local (clean) | `✓ a3f7b2c` | all committed |
| CI/release | `✓ v0.20.0 (a3f7b2c)` | tagged release with commit |

### Rules

- **Local**: short git commit hash (7 chars). Append `dirty` if any tracked `src/**/*.{cpp,h}` files are modified.
- **CI**: version from `VERSION` file + commit hash in parentheses. Detected via `CI=true` env var.
- No network calls — purely local git state.

## Implementation

Single line in Makefile `build:` target using shell substitution.
