---
title: "chore: enable GitHub branch protection for peer review"
labels: chore, sprint-2, cmmi-1
---

## Why (Value Statement)

CMMI 1 check 1.6 requires peer review before merge. Branch protection enforces this at the platform level — no code reaches main without review. This also catches implicit decisions (ADR-048 §11) and improves bus factor.

## ISO 9126 Quality Target

- [x] Reliability

## Acceptance Criteria

```gherkin
Given a pull request targets main
When the PR has zero approvals
Then GitHub blocks the merge

Given a pull request targets main
When CI checks have not passed
Then GitHub blocks the merge
```

## Implementation

See [docs/prompts/07-branch-protection.md](../../docs/prompts/07-branch-protection.md) for exact `gh` CLI command.

## RAID

- **Risks**: Solo developer needs admin override to merge own PRs
- **Dependencies**: None

## References

- ADR-048 §3.3 check 1.6
- Prompt: `docs/prompts/07-branch-protection.md`
