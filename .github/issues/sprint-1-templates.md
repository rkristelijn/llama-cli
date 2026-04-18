---
title: "chore: add GitHub issue and PR templates"
labels: chore, sprint-1, cmmi-0
---

## Why (Value Statement)

Issue templates enforce the Definition of Ready at the source (ADR-048 §6). Without them, tasks enter sprints without value statements or acceptance criteria, breaking the Golden Thread from Strategic → Tactical. This is CMMI 0 check 0.8.

## ISO 9126 Quality Target

- [x] Usability

## Acceptance Criteria

```gherkin
Given a contributor creates a new issue on GitHub
When they select "Feature"
Then they see a template with: Why, ISO 9126 target, AC (Given-When-Then), RAID, DoR checklist

Given a contributor creates a new issue on GitHub
When they select "Bug"
Then they see a template with: description, reproduction steps, expected behavior, environment

Given a contributor opens a pull request
When the PR form loads
Then they see a template with: summary, changes, checklist, decisions section
```

## Implementation

Already created in this sprint:

- `.github/ISSUE_TEMPLATE/feature.md` — feat template with DoR
- `.github/ISSUE_TEMPLATE/bug.md` — fix template with reproduction steps
- `.github/PULL_REQUEST_TEMPLATE.md` — PR checklist with decision catch

## Stacking Effects

1. → Definition of Ready baseline (CMMI 1 check 1.7)
2. → Implicit decision detection via PR "Decisions made" section (CMMI 2 check 2.11)
3. → Sprint planning input (tasks arrive pre-structured)

## RAID

- **Risks**: Contributors may skip template fields — mitigated by checklist format
- **Assumptions**: GitHub renders templates automatically on issue/PR creation
- **Dependencies**: None

## References

- ADR-048 §3.2 check 0.8
- ADR-048 §6 Pre-Sprint Funnel
