# Contributing

## Code style

This codebase is kept accessible for C++ newcomers:

- Write simple C++ — no template magic, no operator overloading, no unnecessary abstractions
- Comments explain *why*, not *what*
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

## Workflow

- Always work on a feature branch, never commit directly to `main`
- Open a pull request to merge — CI must pass before merging
- Bump the version in `VERSION` when merging (see below)

## Verifying your changes

Run the full check suite locally before pushing:

```bash
make quick   # incremental build + tests + comment ratio (fast, for dev)
make test    # full build + all 6 test suites + comment ratio
make check   # everything below — same as CI
```

### What `make check` verifies

| Check | Threshold | Fix |
|-------|-----------|-----|
| Unit tests | all pass | fix the failing test |
| E2E tests | all pass | `e2e/*.sh` — needs Ollama running |
| clang-format | zero violations | `make format` to auto-fix |
| clang-tidy | zero warnings (excl. filtered) | fix the warning or add `NOLINTNEXTLINE` |
| pmccabe | complexity ≤ 10 per function | split the function, or add `pmccabe:skip-complexity` |
| cppcheck | zero errors | fix the issue |
| doxygen | zero warnings | add missing `@param` / `@brief` |
| coverage | ≥ 80% per file | add tests |
| comment ratio | ≥ 20% of lines | add comments explaining *why* |
| semgrep | zero findings | fix the security/quality issue |
| gitleaks | zero secrets | remove the secret, rotate it |

CI runs the same checks automatically on every pull request.

### Configuration

Settings are loaded in this order (last wins):

```
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

This project uses [Semantic Versioning](https://semver.org/):

- `MAJOR.MINOR.PATCH`
- Bump `PATCH` for bug fixes
- Bump `MINOR` for new features
- Bump `MAJOR` for breaking changes

When merging a PR, update the `VERSION` file:

```bash
echo "0.2.0" > VERSION
git add VERSION
git commit -m "chore: bump version to 0.2.0"
```
