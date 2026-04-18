---
title: "chore: add CHANGELOG.md"
labels: chore, sprint-1, cmmi-0
---

## Why (Value Statement)

Users and contributors can't track what changed between releases. A changelog is the Strategic→user trust link: transparent change history enables informed upgrade decisions (ITIL Change Enablement). Once conventional commits are enforced (sprint-1-commit-validation), this becomes auto-generatable at CMMI 3.

## ISO 9126 Quality Target

- [x] Usability

## Acceptance Criteria

```gherkin
Given a user visits the repository
When they open CHANGELOG.md
Then they see changes grouped by version with dates

Given a new release is tagged
When the changelog is reviewed
Then it contains all feat/fix/chore entries since the last release
```

## Implementation

Create `CHANGELOG.md` following [Keep a Changelog](https://keepachangelog.com/) format:
- Group by version tag
- Categories: Added, Fixed, Changed
- Link to GitHub releases

## Stacking Effects

1. → User trust and release transparency
2. → Foundation for auto-generated release notes (CMMI 3 check 3.2)
3. → Feeds the "Transition" phase of the Thin-V (ADR-048 §2)

## RAID

- **Risks**: Historical entries may be incomplete — acceptable, start from current version
- **Dependencies**: None (enhanced by commit validation once available)

## References

- ADR-048 §15 Sprint 1 Plan
- ADR-048 §4 Automation Stacking Matrix (conventional commits → release notes)
- [Keep a Changelog](https://keepachangelog.com/)
