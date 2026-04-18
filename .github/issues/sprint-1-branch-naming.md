---
title: "chore: add branch naming validation"
labels: chore, sprint-1, cmmi-0
---

## Why (Value Statement)

Branch names are the first label on the Operational black box (ADR-048 §14). Without naming conventions, we can't link branches to issues, route CI, or enforce the work type routing (§7). This is CMMI 0 check 0.2.

## ISO 9126 Quality Target

- [x] Maintainability

## Acceptance Criteria

```gherkin
Given a developer is on a branch named "my-feature"
When they make a commit
Then the pre-commit hook rejects with: "Branch must match: (feat|fix|chore|spike)/<issue-id>-<short-name>"

Given a developer is on "feat/42-add-streaming"
When they make a commit
Then the pre-commit hook allows it
```

## Implementation

Add branch name check to `scripts/git/pre-commit.sh` (after the existing `main` block):

- Regex: `^(feat|fix|chore|spike|refactor|docs|test)/[0-9]+-[a-z0-9-]+$`
- Allow `main` (read-only, already blocked for commits)
- Print expected format on failure

## Stacking Effects

1. → Issue linking via branch name (issue ID in branch → auto-link in PR)
2. → CI can route checks by branch prefix (feat gets full pipeline, spike gets minimal)
3. → Traceability: branch → issue → ADR → product vision

## RAID

- **Risks**: Existing branches won't match — only enforced on new commits
- **Assumptions**: Contributors use `make setup` for hooks
- **Dependencies**: None

## References

- ADR-048 §3.2 check 0.2
- ADR-048 §5 Git-Triggered Process Flow
