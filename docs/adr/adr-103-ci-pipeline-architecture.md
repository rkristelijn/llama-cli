---
summary: Restructure CI pipeline into 12 grouped jobs with smart filtering, dependency chains, and commit annotations for skip co
title: "ADR-103: CI Pipeline Architecture"
status: accepted
date: 2026-05-07
---

summary: Restructure CI pipeline into 12 grouped jobs with smart filtering, dependency chains, and commit annotations for skip co

## ADR-103: CI Pipeline Architecture

### Context

The CI pipeline grew organically to 28 separate jobs, causing:

- Duplicate builds (unit-test, e2e-test, feature-coverage each rebuilt from scratch)
- No dependency chain (coverage ran even when tests failed)
- Double triggers (push + PR synchronize both fired)
- No skip mechanism for iterative development
- Heavy jobs (coverage, sanitizers) blocked draft PR feedback

### Decision

Restructure into 12 grouped jobs with smart filtering, dependency chains,
and commit annotations for skip control.

### Pipeline Structure

```text
changes ---+--- lint-code      (format, cppcheck, complexity, docs, ratio, filesize)
           |--- lint-tidy      (clang-tidy, separate: slow)
           |--- lint-docs      (markdown)
           |--- lint-config    (makefile, scripts)
           |--- lint-yaml      (yaml, versions)
           |--- sast           (gitleaks, trufflehog, grype)
           |--- sast-semgrep   (container)
           |--- metrics        (dead-code, dead-docs, duplication)
           |--- build --- test ---+--- feature-coverage
           |                      +--- test-coverage    [heavy]
           |                      +--- sanitizers       [heavy]
           +--- build-macos                             [heavy]
```

### Triggers

```text
Event                    What runs
-----------------------  ------------------------------------------
Push to main             All jobs
PR opened (draft)        Light jobs only (lint, build, test, sast)
PR ready_for_review      Light + heavy (coverage, sanitizers, macOS)
PR synchronize (push)    Same as current PR state (draft or ready)
```

No double triggers: `push:` only fires on `main`, feature branch pushes
only trigger via the `pull_request` event when a PR exists.

### Dependency Chain

```text
build must pass --> test runs
test must pass  --> test-coverage runs
test must pass  --> sanitizers run
test must pass  --> feature-coverage runs
```

If build fails, nothing downstream runs. If tests fail, heavy jobs are skipped.

### Smart Filtering

The `changes` job detects which file types changed:

| Filter | Files | Jobs that run |
|--------|-------|---------------|
| `code` | src/, CMakeLists.txt, Makefile, .config/ | lint-code, build, test |
| `src` | src/, CMakeLists.txt | test-coverage, sanitizers |
| `docs` | *.md | lint-docs |
| `yaml` | *.yml, *.yaml | lint-yaml |
| `scripts` | scripts/, Makefile | lint-config |

### Commit Annotations

Skip specific job groups via commit message:

```text
[skip ci]           Skip entire pipeline (GitHub native)
[skip build]        Skip build + test + coverage
[skip test]         Skip test + coverage (build still runs)
[skip lint]         Skip all lint jobs
[skip sast]         Skip security scans
[only docs]         Only lint-docs runs
[only lint]         Only lint jobs run
```

Combine: `fix: typo [skip build] [skip sast]`

### Heavy Job Gate

Jobs marked `[heavy]` only run when:

- PR is NOT draft (`github.event.pull_request.draft == false`), OR
- Push is to `main`

This is computed once in the `changes` job as `outputs.heavy`.

### Job Grouping Rationale

| Old (28 jobs) | New (12 jobs) | Why grouped |
|---------------|---------------|-------------|
| lint-cpp + lint-cppcheck + complexity + docs + comment-ratio + file-size | lint-code | All need same deps, no build needed |
| lint-makefile + lint-scripts | lint-config | Both check scripts/ |
| lint-yaml + lint-versions | lint-yaml | Both check yaml/config |
| sast-secret + sast-trufflehog + sast-grype + sast-checkov | sast | All security, same runner |
| dead-code + dead-docs + duplication | metrics | All analysis, no build |
| unit-test + e2e-test | test | Sequential in one job, one build |

### Integrity Check

`scripts/lint/check-ci-yaml.sh` validates before commit:

- YAML syntax (yamllint)
- No lines >140 chars (excl. pinned SHAs)
- All `needs.X` references resolve to existing jobs
- No `${{ }}` interpolation in `run:` blocks (security)
- Every `if: needs.X` has a matching `needs:` declaration

Runs automatically in pre-commit hook when `.yml` files are staged.

### Consequences

- 28 jobs reduced to 12 (less GitHub Actions overhead)
- No duplicate builds (test job builds once, runs unit + e2e)
- Dependency chain prevents wasted compute on failures
- Draft PRs get fast feedback (~2 min) without burning heavy credits
- Commit annotations give developers control over iteration speed
- CI integrity check prevents broken pipelines from being pushed

### Target Structure (next iteration)

Each check as individual job, grouped by naming prefix:

```text
changes
├── lint-code (format, cppcheck, complexity, doxygen, ratio, filesize)
├── lint-tidy
├── lint-config (makefile + scripts)
├── lint-docs (markdown)
├── lint-yaml
├── lint-versions
├── integrity-dead-code
├── integrity-dead-docs
├── integrity-duplication
├── integrity-consistency
├── integrity-theme
├── integrity-xref
├── integrity-unicode
├── integrity-portability
├── integrity-slop
├── security-secrets (gitleaks)
├── security-trufflehog
├── security-grype
├── security-semgrep
├── build
│   ├── test-unit
│   │   └── test-coverage [heavy]
│   ├── e2e
│   │   └── feature-coverage [badge]
│   └── sanitizers [heavy]
└── build-macos [heavy]
```

Dependencies:

- All lint-*, integrity-*, security-* depend only on `changes`
- `build` depends on `changes`
- `test-unit` and `e2e` depend on `build`
- `test-coverage` depends on `test-unit` + heavy gate
- `feature-coverage` depends on `e2e`
- `sanitizers` depends on `test-unit` + heavy gate
- `build-macos` depends on `changes` + heavy gate
