---
summary: Implement quality gates at every V-model phase with automated checks starting from planning and ending at release, to en
title: "ADR-101: V-Model Quality Gates"
status: proposed
date: 2026-05-07
---

summary: Implement quality gates at every V-model phase with automated checks starting from planning and ending at release, to en

## ADR-101: V-Model Quality Gates

### Context

We have automated checks at code-level (pre-commit, pre-push, CI) but no gates
at the planning, design, or acceptance phases. This means:

- ADRs can be merged without cross-reference validation
- New features can land without documentation
- Coverage can silently drop
- GCC/Clang differences are caught only after push

### Decision

Define quality gates at every V-model phase, with clear triggers:

```text
Phase           Trigger              Gate (automated)
──────────────  ───────────────────  ─────────────────────────────────
Plan            ADR created          lint-md, check-xref
Design          docs/ changed        lint-md, dead-docs, check-xref
Implement       src/ changed         pre-commit (format, secrets, slop)
Unit Test       git push             pre-push (build, test, comment-ratio)
Integration     PR opened (draft)    CI: lint, build, test, sast
System Test     PR ready for review  CI: full + CodeRabbit + SonarCloud
Acceptance      PR approved          make live-full (manual gate)
Release         merge to main        CI: full + coverage ratchet
```

#### Trigger Points

| Event | Hook/Action | Checks | Duration |
|-------|-------------|--------|----------|
| `git commit` | pre-commit | format, secrets, slop, stegano | ~5s |
| `git commit` | commit-msg | conventional commit format | <1s |
| `git push` | pre-push | build, test-unit, lint, security | ~60s |
| Push to branch | CI (26 jobs) | everything parallel | ~3 min |
| PR opened (draft) | CI + label | same as push (no review tools) | ~3 min |
| PR ready for review | CI + CodeRabbit + Sonar | full analysis + AI review | ~5 min |
| PR approved | manual | `make live-full` confirmation | ~30s |
| Merge to main | CI | full + coverage must not drop | ~10 min |

#### Draft PR vs Ready PR

- **Draft PR**: CI runs but CodeRabbit/SonarCloud skip (saves credits)
- **Ready for review**: full analysis triggers
- Implementation: add `if: github.event.pull_request.draft == false` to expensive jobs

#### Pre-PR Checklist (automated via `make pre-pr`)

```bash
make pre-pr   # New target: runs everything you need before opening a PR
```

Runs:

1. `make build` (Clang, fast)
2. `make build-gcc` (GCC, catches CI errors)
3. `make test-unit` (347 scenarios)
4. `make e2e` (49 features)
5. `make lint` (all linters)
6. `make pipeline-coverage` (no missing targets)
7. `make comment-ratio` (≥20%)

#### Document/Design Phase Checks

When only docs change:

- `make lint-md` (formatting)
- `make check-xref` (ADR cross-references valid)
- `make dead-docs` (no orphaned docs)

When ADRs are added:

- Verify frontmatter (title, status, date)
- Verify INDEX.md is updated

### Consequences

- Every phase has at least one automated gate
- Draft PRs are cheap (no CodeRabbit/Sonar credits burned)
- `make pre-pr` gives confidence before opening PR
- GCC cross-compile catches platform issues before CI
- Coverage ratchet prevents silent regression

### Status

Proposed — implement incrementally:

1. [done] pre-commit, pre-push, CI (already done)
2. [ ] commit-msg hook (script exists, needs `make hooks`)
3. [done] `make pre-pr` target
4. [done] Draft PR skip for heavy checks (`needs.changes.outputs.heavy`)
5. [ ] Coverage ratchet (compare vs main)
6. [ ] Dynamic badges via Gist endpoint (see below)

### Dynamic Badges (like GitLab badge entity)

GitHub has no native badge entities. Use shields.io endpoint + Gist:

1. CI job computes value (test count, coverage %, etc.)
2. Writes JSON to a Gist: `{"schemaVersion":1,"label":"Tests","message":"347","color":"blue"}`
3. README uses: `![](https://img.shields.io/endpoint?url=<gist-raw-url>)`
4. Only updates on merge to main — badge always reflects released state

Setup: create Gist + add `BADGE_TOKEN` (PAT with gist scope) to repo secrets.
Use `schneegans/dynamic-badges-action` in CI.
