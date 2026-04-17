# ADR-026: Tool Version Pinning

*Status*: Accepted · *Date*: 2026-04-12 · *Context*: CI used `ubuntu-latest` and `apt-get install clang-format` without pinning versions. This caused format checks to pass locally (clang-format 21) but fail in CI (clang-format 18), and risks silent breakage when `ubuntu-latest` upgrades.

## Problem

- `ubuntu-latest` maps to a different Ubuntu version over time (currently 24.04, but will change)
- `apt-get install clang-format` installs whatever version is in apt — currently 18, but not guaranteed
- Local macOS uses clang-format 21 via Homebrew — different formatting output
- npm solves this with `package.json` (ranges) + `package-lock.json` (exact pins); no equivalent exists natively for C/C++ projects

## How npm does it

npm separates intent (`package.json`: `"^18.0.0"`) from reproducibility (`package-lock.json`: exact SHA). The lock file is committed and CI uses it.

For C/C++ there is no standard equivalent, but the pattern can be approximated:

| Layer | npm | This project |
|-------|-----|-------------|
| Intent | `package.json` | `Makefile` / `scripts/dev/setup.sh` |
| Pin | `package-lock.json` | Explicit version in CI + `TOOL_VERSIONS` file |
| Install | `npm ci` | `make setup` |

## Decision

1. Pin `ubuntu-24.04` explicitly in `.github/workflows/ci.yml` (not `ubuntu-latest`)
2. Pin `clang-format-18` explicitly in CI install step
3. Add a `TOOL_VERSIONS` file in the repo root documenting expected tool versions
4. `Makefile`: detect `clang-format-18` first, fall back to `clang-format`

## TOOL_VERSIONS format

```text
clang-format=18
cmake=3.28
```

Simple key=value, human-readable, no tooling required to parse.

## Rationale

- Mirrors npm's lock file concept without adding a dependency
- `ubuntu-latest` breakage is a known CI anti-pattern
- clang-format output differs between major versions — pinning is required for reproducible format checks
- `TOOL_VERSIONS` is the single source of truth for expected versions, referenced in `make setup` output

## Consequences

- `.github/workflows/ci.yml`: `ubuntu-latest` → `ubuntu-24.04`, `clang-format` → `clang-format-18`
- `TOOL_VERSIONS` added to repo root
- `Makefile`: `CLANG_FORMAT = $(shell command -v clang-format-18 2>/dev/null || command -v clang-format)`
- `scripts/dev/setup.sh`: install `clang-format-18` explicitly on Linux
