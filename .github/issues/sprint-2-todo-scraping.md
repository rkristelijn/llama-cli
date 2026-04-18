---
title: "chore: automate TODO scraping to TECHDEBT.md"
labels: chore, sprint-2, cmmi-1
---

## Why (Value Statement)

CMMI 1 check 1.3 requires tech debt to be visible and tracked. `make todo` already scans for TODOs but only displays them. Writing to `TECHDEBT.md` makes debt a first-class artifact that feeds sprint planning (chess principle: one check, two purposes).

## ISO 9126 Quality Target

- [x] Maintainability

## Acceptance Criteria

```gherkin
Given a developer runs "make todo"
When TODOs exist in source code or markdown
Then TECHDEBT.md is generated with counts and file references
```

## Implementation

See [docs/prompts/06-todo-scraping.md](../../docs/prompts/06-todo-scraping.md) for exact code.

## RAID

- **Dependencies**: None

## References

- ADR-048 §3.3 check 1.3
- ADR-048 §4 Automation Stacking Matrix
- Prompt: `docs/prompts/06-todo-scraping.md`
