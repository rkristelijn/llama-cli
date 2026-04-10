# ADR-006: Branching and Release Strategy

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: A branching and release strategy is needed. Git Flow was considered but adds overhead that does not pay off for a single-developer project.

## Decision
GitHub Flow is used: feature branches → PR → main. Releases are tagged manually at milestones.

### Branching
- `main` is always deployable
- All work happens on feature branches (`feat/`, `fix/`, `refactor/`, etc.)
- PRs require CI green before merge
- Direct commits to main are blocked by pre-commit hook

### Releases
- VERSION is bumped on every PR
- Git tags (`v0.3.0`) and GitHub Releases are created at milestones (e.g. MVP)
- No `develop` or `release` branches

## Rationale
- Git Flow's develop/release/hotfix branches add overhead without benefit for a single developer
- GitHub Flow is the simplest strategy that supports CI and code review
- Tagging at milestones gives meaningful releases without continuous release pressure

## Consequences
- No scheduled releases — releases happen when milestones are reached
- If multiple developers join, this strategy may need to be revisited
