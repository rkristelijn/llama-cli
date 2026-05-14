---
summary: The ADR proposal effectively outlines a two-layer caching strategy for CI build caching across multiple providers, including GitHub Actions, GitLab CI, Codeberg/Forgejo, and self-hosted runners, aiming to reduce pipeline time by 40-60%.
status: proposed
---

# ADR-122: CI Build Caching and Portability Strategy

## Context

Current CI pipeline runs 8-12 minutes per push. The `lint-code` (cppcheck) job sometimes hangs for 2+ hours on GitHub's shared runners. Build artifacts are recreated from scratch every run even when source hasn't changed. Meanwhile, we want to avoid vendor lock-in to GitHub Actions specifically.

## Decision

Implement a two-layer caching strategy that is portable across CI providers.

### Layer 1: Build artifact cache (cmake output)

Cache the `build/` directory keyed on source hash. If C++ source hasn't changed, skip the entire cmake build.

| Provider | Mechanism | Lock-in risk |
|----------|-----------|-------------|
| GitHub Actions | `actions/cache@v4` | Low — standard action, easy to replace |
| GitLab CI | `cache:` directive (built-in) | None — native feature |
| Forgejo/Codeberg | Same as GitHub Actions (compatible) | None |
| Self-hosted | Local filesystem persistence | None |

### Layer 2: Smart filtering (only run what changed)

Already partially implemented via `dorny/paths-filter`. Extend to:

| Changed files | Jobs that run | Jobs skipped |
|---------------|--------------|-------------|
| `src/`, `CMakeLists.txt` | build, test, lint-code, lint-tidy, coverage, sanitizers | lint-docs, lint-config |
| `scripts/`, `Makefile` | lint-scripts, lint-config | build, test, coverage |
| `*.md` | lint-docs | everything else |
| `*.yml` | lint-config | everything else |
| `.config/` only | lint-config | everything else |

### Layer 3: Incremental analysis (future)

Run cppcheck/clang-tidy only on changed files in CI (same as local `make tidy` smart mode).

## Expected savings

| Current | With caching | Saving |
|---------|-------------|--------|
| Build: 57s | Build: 5s (cache hit) | -52s |
| lint-tidy: 164s | lint-tidy: 30s (fewer files) | -134s |
| lint-code: 60s-7200s | lint-code: 10s (changed files only) | -50s to -7190s |
| Total: ~8 min | Total: ~3 min | **~60% faster** |

## Vendor lock-in analysis

### GitHub Actions specifics we use

| Feature | Portable? | Alternative |
|---------|-----------|-------------|
| `actions/cache` | Yes — standard pattern | GitLab `cache:`, local dirs |
| `dorny/paths-filter` | Partially — uses GitHub API | `git diff --name-only` in script |
| `actions/checkout` | Yes — just git clone | `git clone` |
| `ubuntu-24.04` runner | Yes — any Linux | Docker, self-hosted |
| Secrets (`${{ secrets.X }}`) | Yes — all CI have this | env vars |
| `gh pr` CLI | GitHub-only | API calls, or `glab` for GitLab |

### Truly GitHub-locked features

1. **GitHub-specific Actions** (dorny/paths-filter, codecov/action) — replaceable with scripts
2. **Status checks API** — each provider has their own
3. **PR comments** (CodeRabbit, SonarCloud) — webhook-based, provider-agnostic

### Migration path to other hosts

```bash
# The Makefile IS the portable CI layer:
make check          # same on any host
make build          # same everywhere
make test           # same everywhere
```

The CI YAML is thin — it just calls `make` targets. Migrating to GitLab/Forgejo means rewriting ~50 lines of YAML, not the actual checks.

## European alternatives

| Host | CI | Cache | Status |
|------|-----|-------|--------|
| **Codeberg** (Forgejo) | Forgejo Actions (GitHub-compatible) | Same syntax | Production-ready |
| **GitLab** (EU hosting available) | GitLab CI | Built-in, superior | Production-ready |
| **Sourcehut** | builds.sr.ht | Manual | Minimal but functional |
| **Self-hosted Gitea** | Gitea Actions | Same as GitHub | Full control |

Codeberg is the easiest migration — Forgejo Actions are 95% compatible with GitHub Actions YAML.

## Implementation plan

1. Add `actions/cache` for `build/` directory (keyed on `hashFiles('CMakeLists.txt', 'src/**')`)
2. Make `lint-code` smart in CI (pass changed files list)
3. Document the `make` targets as the portable interface (CI YAML is just glue)

## Consequences

- Pipeline drops from ~8 min to ~3 min on cache hits
- No vendor lock-in — `make check` works anywhere
- Migration to Codeberg/GitLab requires only YAML rewrite, not logic changes
- Caching adds complexity (cache invalidation) but cmake hash key is reliable

## References

- @see .github/workflows/ci.yml
- @see docs/adr/adr-103-ci-pipeline-architecture.md
- @see CONTRIBUTING.md (smart CI filtering table)
