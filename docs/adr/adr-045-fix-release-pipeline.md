# ADR-045: Fix Release Pipeline

*Status*: Proposed · *Date*: 2026-04-17

## Context

The release pipeline (`.github/workflows/release.yml`) has several bugs:

1. **Version output is broken** — `echo "version=$VERSION"` uses `$VERSION` before it's set; the correct value is on the previous line via `cat VERSION`. Result: tag is `v` instead of `v0.x.y`.
2. **Artifact naming** — binaries are uploaded as bare names (`llama-cli-linux-x64`) but should be tarballed for download (users expect `.tar.gz` or at minimum a consistent format).
3. **Cross-compile cmake_flags** — `matrix.cmake_flags` is undefined for non-cross-compile targets, causing potential issues.
4. **Runner pinning** — uses `ubuntu-latest` instead of `ubuntu-24.04` (same issue ADR-44 fixed for CI).
5. **Action pinning** — uses mutable tags (`@v4`, `@v2`) instead of commit SHAs.

## Decision

Fix the release workflow:

1. Fix version output: `VERSION=$(cat VERSION)` then use `$VERSION` consistently
2. Package binaries as `llama-cli-<version>-<os>-<arch>.tar.gz`
3. Default `cmake_flags` to empty string in matrix
4. Pin `ubuntu-24.04` and action SHAs
5. Use `scripts/ci/install-deps.sh` for git-cliff installation

## Acceptance Criteria

- [ ] `VERSION` file containing `0.x.y` produces tag `v0.x.y` and release title `Release v0.x.y`
- [ ] Release artifacts are named `llama-cli-<version>-linux-x64.tar.gz`, `llama-cli-<version>-linux-arm64.tar.gz`, `llama-cli-<version>-macos-arm64.tar.gz`
- [ ] Runner pinned to `ubuntu-24.04`, actions pinned to SHAs
- [ ] README download instructions match actual artifact names

## References

- @see docs/adr/adr-44-tidy-boilerplate.md — CI pinning rationale
- @see .github/workflows/release.yml
