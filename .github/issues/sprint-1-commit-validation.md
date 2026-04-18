---
title: "chore: add commit message validation (conventional commits)"
labels: chore, sprint-1, cmmi-0
---

## Why (Value Statement)

Conventional commits are the foundation of the Golden Thread (ADR-048 §1). Without enforced commit format, we can't auto-generate changelogs, route CI checks by type, or trace commits to their purpose. This is CMMI 0 check 0.1.

## ISO 9126 Quality Target

- [x] Maintainability

## Acceptance Criteria

```gherkin
Given a developer makes a commit
When the message does not match "type(scope): description" or "type: description"
Then the commit is rejected with a helpful error message

Given a developer makes a commit
When the message matches "feat: add streaming support"
Then the commit succeeds
```

## Implementation

Add `scripts/git/commit-msg.sh`:
- Validate against regex: `^(feat|fix|chore|refactor|docs|test|ci|style|perf|build)(\(.+\))?: .{1,72}$`
- Print example on failure
- Install via `make hooks` (symlink to `.git/hooks/commit-msg`)

## Stacking Effects

1. → Enables auto-changelog generation (CMMI 3 check 3.2)
2. → Enables CI routing by commit type
3. → Feeds release note generation

## RAID

- **Risks**: None (local hook, non-destructive)
- **Assumptions**: All contributors run `make setup` which installs hooks
- **Dependencies**: None

## References

- ADR-048 §3.2 check 0.1
- ADR-048 §4 Automation Stacking Matrix
