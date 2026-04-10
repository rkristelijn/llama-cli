# ADR-003: Development Workflow

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: A structured way of working is needed to ensure quality from idea to release. This workflow combines proven practices: define before build (V-model), tests before code (TDD), document decisions (ADRs), and no waste (Lean).

## Decision
The following workflow is followed for every feature:

### Level 1 — Overview

```mermaid
graph LR
    Define --> Build --> Verify --> Release
```

### Level 2 — Define

```mermaid
graph LR
    A[Value discussion] --> B[Options analysis]
    B --> C[Decision + ADR]
    C --> D[Design + diagrams]
    D --> E[Write tests]
```

### Level 2 — Build

```mermaid
graph LR
    E[Write tests] --> F[Implement minimal code]
```

### Level 2 — Verify

```mermaid
graph LR
    F[Unit test] --> G[Integration test]
    G --> H[E2E test]
    H --> I[Regression test]
```

### Level 2 — Release

```mermaid
graph LR
    J[Local checks] --> K[Update docs]
    K --> L[Refactor if needed]
    L --> M[PR + merge]
    M --> N[Release]
```

### Phases

| # | Phase | Output |
|---|-------|--------|
| 1 | **Value discussion** | Why does this feature matter? What problem is solved? |
| 2 | **Options analysis** | What are the possible approaches? Pros/cons of each |
| 3 | **Decision + ADR** | One option is picked, rationale is documented in `docs/adr-xxx-*.md` |
| 4 | **Design** | Mermaid diagrams are added to docs (sequence, class, flowchart) |
| 5 | **Write tests** | TDD — tests are written first, based on the design |
| 6 | **Implement** | Minimal code is written to make tests pass |
| 7 | **Unit test** | Individual components are verified |
| 8 | **Integration test** | Components are verified to work together |
| 9 | **E2E test** | The full user flow is verified |
| 10 | **Regression test** | Nothing else is broken |
| 11 | **Local checks** | `make test && make check` are run |
| 12 | **Update docs** | README, docs/README.md, ADRs are updated |
| 13 | **Refactor** | Code is cleaned up if needed, tests are re-run |
| 14 | **PR + merge** | Feature branch → PR → CI green → merge |
| 15 | **Release** | Version is bumped, tag and changelog are created (TBD) |

## Rationale
- **Left side first**: thinking before coding prevents wasted effort
- **TDD**: tests define the contract before implementation
- **Right side mirrors left**: each verification level corresponds to a definition level
- **Discipline over speed**: structure matters — this is a code-flex

## Influences
- [V-model](https://en.wikipedia.org/wiki/V-model) — define left, verify right
- [TDD](https://en.wikipedia.org/wiki/Test-driven_development) — tests before code (Kent Beck)
- [ADRs](https://adr.github.io/) — document decisions (Michael Nygard)
- [Lean](https://en.wikipedia.org/wiki/Lean_software_development) — no waste, only what adds value

## Consequences
- Features take longer to start but are higher quality
- Documentation exists before code is written
- Test coverage grows naturally with every feature
- The release process still needs to be defined (ADR to follow)
