# Contributing

## Workflow

- Always work on a feature branch, never commit directly to `main`
- Open a pull request to merge — CI must pass before merging
- Bump the version in `VERSION` when merging (see below)

## Verifying your changes

Run the full check suite locally before pushing:

```bash
make test    # comment ratio check
make check   # cppcheck + semgrep + gitleaks
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
