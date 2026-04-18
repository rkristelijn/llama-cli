---
title: "chore: bump CI coverage threshold to 60%"
labels: chore, sprint-2, cmmi-1
---

## Why (Value Statement)

CMMI 1 check 1.1 requires ≥ 60% test coverage. Current CI threshold is 55%. This ensures shipped code has meaningful test coverage, reducing regression risk and building confidence for the streaming feature.

## ISO 9126 Quality Target

- [x] Reliability

## Acceptance Criteria

```gherkin
Given the CI pipeline runs on a pull request
When the test coverage is below 60%
Then the pipeline fails with "Coverage below 60%!"
```

## Implementation

See [docs/prompts/05-coverage-bump.md](../../docs/prompts/05-coverage-bump.md) for exact code.

## RAID

- **Risks**: If current coverage is between 55-60%, CI will fail until more tests are added
- **Dependencies**: None

## References

- ADR-048 §3.3 check 1.1
- Prompt: `docs/prompts/05-coverage-bump.md`
