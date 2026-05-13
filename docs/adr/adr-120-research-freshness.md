---
summary: Track last-researched dates per topic in .config/research-dates.env; warn during make lint when any topic exceeds 30 days with a ready-to-use research prompt. Covers slop detection, model guide, security tools, and portability.
status: accepted
---

# ADR-120: Research Freshness Tracking

## Context

Several scripts depend on external knowledge that evolves monthly: AI slop patterns, model benchmarks, security tool versions, and portability gotchas. Without periodic re-research, detection rules go stale and miss new patterns. We need a lightweight reminder system that integrates into the existing lint workflow.

## Decision

Track last-researched dates in `.config/research-dates.env` and warn during `make lint` when any topic exceeds 30 days. Each warning includes a ready-to-use research prompt so the developer (or AI agent) can immediately start the research cycle.

## Mechanism

```text
.config/research-dates.env     ← KEY=YYYY-MM-DD per topic
scripts/lint/check-research-freshness.sh  ← reads dates, warns if stale
make research-freshness        ← runs the check (part of `make lint`)
make research-update TOPIC=X   ← updates date after research is done
```

### Flow

1. `make lint` runs `research-freshness` as part of the aggregator.
2. If a topic is >30 days old, the script prints a boxed research prompt.
3. Developer performs the research (web search, paper review, tool updates).
4. Developer updates the relevant scripts/docs with findings.
5. Developer runs `make research-update TOPIC=SLOP_DETECTION` to reset the clock.

### Adding a new topic

1. Add a line to `.config/research-dates.env`: `NEW_TOPIC=YYYY-MM-DD`
2. Add a case to `prompt_for()` in `check-research-freshness.sh` with the research prompt.

## Current Topics

| Key | Scripts affected | Research scope |
|-----|-----------------|----------------|
| SLOP_DETECTION | check-slop.sh | AI code patterns, tools, word lists |
| MODEL_GUIDE | model-guide.md, bench-models.sh | New models, benchmarks, hardware |
| SECURITY_TOOLS | scripts/security/* | Tool versions, CVE patterns |
| PORTABILITY | check-portability.sh | Shell/OS compatibility patterns |

## Design Choices

- **Non-blocking** — warnings only, never fails the build.
- **Portable date math** — handles both macOS (`date -j`) and Linux (`date -d`).
- **Prompt-driven** — each warning contains a specific, actionable research prompt.
- **Single source of truth** — all dates in one file, easy to audit.
- **30-day default** — monthly cadence balances freshness vs. overhead.

## Consequences

- `make lint` reminds developers when knowledge is stale.
- Research becomes a tracked, repeatable process rather than ad-hoc.
- New topics can be added in 2 lines (date + prompt).
- The system is self-documenting: the prompt tells you exactly what to research.

## References

- @see .config/research-dates.env
- @see scripts/lint/check-research-freshness.sh
- @see docs/adr/adr-119-slop-detection.md
