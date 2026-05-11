---
summary: Automated Branch Cleanup
status: accepted
---

# ADR-115: Automated Branch Cleanup

## Context

Merged branches accumulate over time (28 local, 47 remote at time of writing). They clutter `git branch` output and GitHub's branch list, making it harder to find active work.

## Decision

Add `make gh-cleanup` (script: `scripts/gh/gh-cleanup.sh`) that removes branches already merged into main.

### Behavior

- **Dry-run by default** — shows what would be deleted without acting
- **`--apply`** — actually deletes branches
- **Skips** — `main`, current branch
- **Uses `--no-verify`** — skips pre-push hooks on `git push --delete` (no point running lint/test for a branch deletion)

### Usage

```bash
make gh-cleanup              # dry-run
make gh-cleanup ARGS=--apply # delete merged branches
```

## Consequences

- Keeps branch list clean and navigable
- Safe: only deletes branches already in main (recoverable via reflog for 90 days)
- Run periodically or after merging a batch of PRs
