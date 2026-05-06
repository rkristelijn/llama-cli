# ADR-095: Bidirectional Traceability via Feature Registry

**Status:** Implemented · **Date:** 2026-05-06 · **Extends:** [ADR-063](adr-063-dynamic-runtime-feature-coverage.md), [ADR-048](adr-048-quality-framework.md)

## Context

ADR-048 defines the "Golden Thread": every commit traces back to a strategic "Why" (an ADR). ADR-063 introduced `LOG_FEATURE()` macros to prove that E2E tests exercise code paths. However, there is no automated check that:

1. Every `LOG_FEATURE` token links to a documented requirement (ADR).
2. Every ADR with status "Implemented" has at least one `LOG_FEATURE` token proving it exists in code.

The existing `doc-coverage.sh` script uses naive name-grep (99% match, 169 false-positive orphans) and is insufficient for CMMI audit purposes.

## Problem Statement

For CMMI Level 1+ compliance (ADR-048 §1.8, §Golden Thread):

- **Code → Docs**: every feature in code must be traceable to a design decision.
- **Docs → Code**: every documented decision must be verifiable in the implementation.

Without this, documentation drifts from reality and audits cannot confirm coverage.

## Decision

Introduce a **feature registry** (`docs/feature-registry.yml`) that explicitly maps each `LOG_FEATURE` token to one or more ADRs, plus a CI script that enforces three invariants:

| # | Check | Direction | Failure means |
|---|-------|-----------|---------------|
| 1 | Every `LOG_FEATURE` token in `src/` exists in the registry | Code → Docs | New feature added without documentation link |
| 2 | Every registry entry references an existing ADR file | Docs exist | Registry points to non-existent documentation |
| 3 | Every ADR with status `Implemented` appears in at least one registry entry | Docs → Code | Documented decision has no implementation proof |

## Implementation

### Feature Registry Format

```yaml
# docs/feature-registry.yml
# Maps LOG_FEATURE tokens to their governing ADR(s).
repl_mode:
  adr: [ADR-012]
  description: "Interactive REPL mode"
write_annotation:
  adr: [ADR-014, ADR-019]
  description: "LLM proposes file write"
```

### CI Script

`scripts/ci/check-traceability.sh` — runs the three checks, exits non-zero on failure.

### Integration

- Make target: `make traceability`
- CI job: runs after build, alongside `feature-coverage`

## Consequences

### Positive

- Auditable bidirectional traceability (CMMI requirement).
- Drift detection: new `LOG_FEATURE` without registry entry fails CI.
- Dead documentation detection: ADR without implementation proof is flagged.
- Builds on existing `LOG_FEATURE` infrastructure — no code changes needed.

### Negative

- Developers must update `feature-registry.yml` when adding a `LOG_FEATURE`.
- Registry maintenance is manual (but CI enforces it).

### Neutral

- Replaces `doc-coverage.sh` as the authoritative traceability check.
- The existing `check-feature-coverage.sh` (runtime proof) remains complementary.

## Relationship to Other ADRs

| ADR | Relationship |
|-----|-------------|
| ADR-048 | Defines the Golden Thread requirement this ADR implements |
| ADR-063 | Provides the `LOG_FEATURE` macro infrastructure we build upon |
| ADR-027 | Event logging framework that `LOG_FEATURE` uses internally |
