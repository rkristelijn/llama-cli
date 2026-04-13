# Contributing

## Code style

This codebase is kept accessible for C++ newcomers:

- Write simple C++ — no template magic, no operator overloading, no unnecessary abstractions
- Comments explain *why*, not *what*
- Test names read as documentation: `TEST_CASE("config: env vars override defaults")`
- Prefer `std::string` over `const char*`, `std::vector` over raw arrays
- Avoid chained `<<` operators across multiple lines — CI clang-format (v14) and local (v21+) disagree on line-breaking. Use separate statements instead
- If a beginner can't understand it in 30 seconds, simplify it

## Workflow

- Always work on a feature branch, never commit directly to `main`
- Open a pull request to merge — CI must pass before merging
- Bump the version in `VERSION` when merging (see below)

## Verifying your changes

Run the full check suite locally before pushing:

```bash
make quick   # incremental build + tests + comment ratio (fast, for dev)
make test    # full build + all 6 test suites + comment ratio
make check   # everything: clang-tidy, pmccabe, cppcheck, doxygen, index freshness, coverage, semgrep, gitleaks
```

CI runs the same checks automatically on every pull request.

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
