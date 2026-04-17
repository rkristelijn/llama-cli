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

## Verifying your changes (Dev-First)

We follow a **Dev-First** philosophy: productivity and fast feedback loops for developers take precedence over exhaustive automation on every commit.

### Why Dev-First?
- **Immediate Feedback**: You shouldn't wait 5 minutes for a full static analysis of 100 files when you only changed one.
- **Lower Cognitive Load**: By focusing only on your changes, you can fix issues as you write them.
- **Sustainability**: Faster local checks mean more frequent verification and fewer broken CI builds.

### Check targets

```bash
make check       # SMART & FAST: Only verifies files changed vs main (default)
make full-check  # EXHAUSTIVE: Verifies every file in the project (CI uses this on main)
make quick       # Incremental build + unit tests + comment ratio
make live        # Integration test with real LLM (requires running Ollama)
```

### What is verified

| Check | Tool | Smart Mode | target |
|-------|------|------------|--------|
| Unit tests | doctest | Always runs all | `make test` |
| E2E tests | bash | Always runs all | `make e2e` |
| Format | clang-format | Always runs all | `make format-check` |
| clang-tidy | clang-tidy | Only changed files | `make tidy` |
| pmccabe | pmccabe | Always runs all | `make complexity` |
| cppcheck | cppcheck | Always runs all | `make lint` |
| doxygen | doxygen | Always runs all | `make docs` |
| coverage | gcov | Folder-level summary | `make coverage-folder` |
| comment ratio | cloc | Always runs all | `make comment-ratio` |
| SAST Security | semgrep | Always runs all | `make sast-security` |
| SAST Secret | gitleaks | Always runs all | `make sast-secret` |

CI runs `make check` on pull requests to ensure your changes are clean, and `make full-check` on the `main` branch to maintain absolute project integrity.

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
