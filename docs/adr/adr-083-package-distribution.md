# ADR-083: Package Distribution

*Status*: Proposed · *Date*: 2026-05-05 · *Issue*: [#39](https://github.com/rkristelijn/llama-cli/issues/39)

## Context

Users must build from source or use the install script. No native package manager support exists. The install script handles Linux/macOS binary downloads but doesn't integrate with system package managers.

## Decision

Provide native package distribution via:

1. **Homebrew tap** — `brew install rkristelijn/tap/llama-cli`
2. **apt/deb** — `.deb` package or PPA for Debian/Ubuntu
3. **Static binary** — fully static Linux build (current approach via install.sh)

CI builds release artifacts for all targets automatically.

## Consequences

- Lower barrier to entry for new users
- Automatic updates via package managers
- Additional CI complexity for multi-platform builds
- Must maintain formula/package definitions

## References

- [ADR-045: Fix Release Pipeline](adr-045-fix-release-pipeline.md)
