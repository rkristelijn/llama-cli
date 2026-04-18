---
title: "ci: add doc-change enforcement check"
labels: ci, sprint-2, cmmi-1
---

## Why (Value Statement)

CMMI 1 check 1.8 requires documentation to stay current when user-facing code changes. A CI warning catches forgotten doc updates before merge, keeping the knowledge base accurate (ADR-048 §11 KT Rule).

## ISO 9126 Quality Target

- [x] Usability

## Acceptance Criteria

```gherkin
Given a PR changes files in src/
When no files in docs/ or CHANGELOG.md are changed
Then CI posts a warning: "Source code changed but no documentation was updated"
```

## Implementation

See [docs/prompts/08-doc-change-ci.md](../../docs/prompts/08-doc-change-ci.md) for exact CI YAML.

## RAID

- **Risks**: False positives on internal-only changes — mitigated by using warning, not failure
- **Dependencies**: None

## References

- ADR-048 §3.3 check 1.8
- Prompt: `docs/prompts/08-doc-change-ci.md`
