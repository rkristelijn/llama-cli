# Contributing

## Code style

This codebase is kept accessible for C++ newcomers:

- Write simple C++ — no template magic, no operator overloading, no unnecessary abstractions
- Write booleans in positive, so allow, (not supress or deny), so you get !allowed instead of double negation.
- Comments explain *why*, not *what* — we enforce a **minimum 20% comment ratio** in all code and scripts (`make comment-ratio`)
- Test names read as documentation: `TEST_CASE("config: env vars override defaults")`
- Prefer `std::string` over `const char*`, `std::vector` over raw arrays
- Avoid chained `<<` operators across multiple lines — CI clang-format (v14) and local (v21+) disagree on line-breaking. Use separate statements instead
- If a beginner can't understand it in 30 seconds, simplify it

### clang-tidy suppression vs clang-format

When you need to suppress a clang-tidy warning, use `NOLINTNEXTLINE` on the line *above* the target — not `NOLINT` at the end of the line. End-of-line `NOLINT` comments extend the line length, which causes clang-format to reflow the line and break formatting.

```cpp
// ✅ Good — clang-format can freely format the function signature
// NOLINTNEXTLINE(readability-function-size)
static void parse_args(int argc, const char* const argv[], Config& c) {

// ❌ Bad — NOLINT pushes the line past the column limit, clang-format fights back
static void parse_args(int argc, const char* const argv[],
                       Config& c) {  // NOLINT(readability-function-size)
```

Quick reference:

| Comment | Scope | When to use |
|---------|-------|-------------|
| `// NOLINTNEXTLINE(check)` | next line | Default choice — keeps formatting clean |
| `// NOLINT(check)` | same line | Only for short lines that won't exceed column limit |
| `// NOLINTBEGIN(check)` / `// NOLINTEND` | block | Larger sections (use sparingly) |

### doctest macros and clang-format

`SCENARIO`, `GIVEN`, `WHEN`, `THEN`, `TEST_CASE` are uppercase macros — clang-format must not lowercase them. They are listed in `StatementMacros` in `.config/.clang-format`. If you add a new doctest macro, add it there too. Never run `clang-tidy --fix` on test files — it does not understand doctest macros and will corrupt them.

## Shell scripts

All shell scripts follow the conventions in [docs/tools/shell-scripts.md](docs/tools/shell-scripts.md). Summary:

- **Bash only** — `#!/usr/bin/env bash`, never `#!/bin/sh` or `#!/bin/zsh`
- **Safety flags** — every script starts with `set -o errexit`, `set -o nounset`, `set -o pipefail`
- **Debug support** — every script includes `if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi`
- **Naming** — kebab-case filenames with `.sh` extension: `check-format.sh`, not `check_format.sh`
- **Organization** — scripts live in subdirectories by purpose: `scripts/check/`, `scripts/ci/`, `scripts/dev/`, `scripts/gh/`, `scripts/test/`
- **Comments** — file header required (purpose, usage, environment), function headers for non-trivial functions (Google-style: Globals/Arguments/Outputs/Returns)
- **Linting** — all scripts must pass ShellCheck. Run `make shellcheck` to verify
- **`main()` pattern** — scripts with multiple functions use a `main()` entry point called at the bottom: `main "$@"`

### Makefile conventions

The Makefile is an **orchestration layer**, not a scripting environment:

- **Simple targets stay inline** — `clean`, `install`, `format` (1–5 lines, no conditionals)
- **Complex logic goes to scripts** — if a recipe has conditionals, loops, or exceeds ~5 lines, extract it to `scripts/`
- **All targets have `##` comments** — used by `make help` for auto-generated documentation
- **Aliases are documented** — shorthand targets (e.g., `s` for `start`) appear in help output
- **`make help` is the default** — bare `make` shows help, not builds

### CI conventions

CI workflows are thin proxies that call `make` targets or scripts:

- **No inline build logic** — CI YAML handles triggers, runners, caching, and secrets only
- **Pin runner OS** — `ubuntu-24.04`, never `ubuntu-latest`
- **Pin action versions** — use commit SHA with version comment, not mutable tags
- **Reproducible locally** — `make check` runs the same checks as CI
- **Smart filtering** — jobs only run when relevant files change (see table below)

### Smart CI and hook filtering

Git hooks and CI jobs detect which file types changed and skip irrelevant checks:

| Changed files | Pre-commit runs | Pre-push runs | CI runs |
|---------------|----------------|---------------|---------|
| `src/`, `CMakeLists.txt` | format-code | build, test, tidy, e2e, comment-ratio | all code jobs + coverage + sanitizers |
| `scripts/`, `Makefile` | format-scripts | lint-makefile, lint-scripts | lint-makefile, lint-scripts |
| `*.md` | format-md | — | lint-markdown |
| `*.yml` / `*.yaml` | format-yaml | — | lint-yaml |
| any | index, todo, sast-secret | sast-security | sast-secret |

CI uses `dorny/paths-filter` with 5 filters (`code`, `src`, `docs`, `yaml`, `scripts`). Git hooks use `git diff --cached` (pre-commit) and `git diff main...HEAD` (pre-push) to detect file types.

### Version pinning

All development tools are pinned in `.config/versions.env` (single source of truth):

- **`.config/versions.env`** — shell-sourceable `KEY=VALUE` file, read by Makefile and `scripts/dev/setup.sh`
- **`make setup`** — installs all tools at pinned versions, works on macOS (brew) and Linux (apt)
- **`make check-versions`** — warns when installed versions don't match `.config/versions.env`
- **No Node.js or Python required** — markup linting uses native binaries: `yamllint` (installed via brew/apt) and `rumdl` (Rust single-binary). All config lives in `.config/`
- See [ADR-026](docs/adr/adr-026-version-pinning.md) for rationale

## Workflow

- Always work on a feature branch, never commit directly to `main`
- Open a pull request to merge — CI must pass before merging
- Bump the version in `VERSION` when merging (see below)

## Verifying your changes (Dev-First)

We follow a **Dev-First** philosophy: productivity and fast feedback loops for developers take precedence over exhaustive automation on every commit.

### Why Dev-First?

- **Immediate Feedback**: You shouldn't wait 5 minutes for a full static analysis of 100 files when you only changed one.
- **Lower Cognitive Load**: By focusing only on your changes, you can fix issues as you write them.
- **Sustainability**: Faster local checks mean more frequent verification and fewer broken CI builds.

### Check targets

```bash
make check       # DEFAULT: Full linting, smart clang-tidy (changed files only)
make full-check  # EXHAUSTIVE: Verifies every file in the project (CI uses this on main)
make quick       # Incremental build + unit tests + comment ratio
make live        # Integration test with real LLM (requires running Ollama)
```

### What is verified

Ordered by the [shift-left principle](https://en.wikipedia.org/wiki/Shift-left_testing): fastest checks first, no-build checks before build-dependent checks.

| Phase | Check | Tool | Smart Mode | target |
|-------|-------|------|------------|--------|
| Lint | Format | clang-format | Always runs all | `make cpp-format` |
| Lint | YAML lint | yamllint | Always runs all | `make yamllint` |
| Lint | Markdown lint | rumdl | Always runs all | `make markdownlint` |
| Analysis | clang-tidy | clang-tidy | Only changed files | `make tidy` |
| Analysis | Complexity | pmccabe | Always runs all | `make complexity` |
| Analysis | cppcheck | cppcheck | Always runs all | `make lint` |
| Analysis | Doxygen | doxygen | Always runs all | `make docs` |
| Test | Unit tests | doctest | Always runs all | `make test` |
| Test | E2E tests | bash | Always runs all | `make e2e` |
| Test | Coverage | gcov | Folder-level summary | `make coverage-folder` |
| Security | SAST Security | semgrep | Always runs all | `make sast-security` |
| Security | SAST Secret | gitleaks | Always runs all | `make sast-secret` |
| Metrics | Comment ratio | cloc | Always runs all | `make comment-ratio` |

CI runs `make check` on pull requests to ensure your changes are clean, and `make full-check` on the `main` branch to maintain absolute project integrity.

### Configuration

Settings are loaded in this order (last wins):

```text
struct defaults → environment variables → .env file → CLI arguments
```

Put project-local settings in `.env` (git-ignored):

```bash
OLLAMA_HOST=localhost:11434
OLLAMA_MODEL=gemma4:26b
TRACE=true
```

See `src/config/config.h` for all available settings.

## Versioning

This project uses [Semantic Versioning](https://semver.org/) derived from git tags and conventional commits:

- `fix:` commits → patch bump
- `feat:` commits → minor bump
- `feat!:` or `BREAKING CHANGE` → major bump

Releases are triggered manually via GitHub Actions → Release → "Run workflow". The pipeline analyzes commits since the last tag and auto-determines the version.

To preview or create a tag locally:

```bash
make bump            # auto-detect from commits
make bump PART=minor # force a specific bump
```
