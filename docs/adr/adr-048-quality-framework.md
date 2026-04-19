# ADR-048: AI-Guided Quality Framework

*Status*: Accepted · *Date*: 2026-04-18 · *Extends*: [ADR-047](adr-047-ai-guided-development-qa.md)

## Context

ADR-047 explored the ideas behind an AI-guided development QA process. This ADR distills those ideas into a concrete, adoptable framework: the **Lean Quality Framework (LQF)**. It defines maturity levels, automation matrices, and the Operational/Tactical/Strategic pillars that connect daily commits to long-term product vision.

The chess principle applies: **every move must serve at least two purposes**. A linting suppression becomes a TODO comment, which feeds the tech-debt metric, which informs the sprint backlog. Nothing exists in isolation.

## Decision

Adopt the Lean Quality Framework structured around three pillars and four CMMI-inspired maturity levels, with stacking automation where every check serves multiple purposes.

---

## 1. The Three Pillars

### 1.1 Operational — "How" — Doing things right

| Aspect | Detail |
|--------|--------|
| Focus | Day-to-day execution: the "Thin-V" loop, immediate service stability |
| Activities | Coding (feat/fix/chore), TDD, unit testing, incident response, peer reviews |
| Metrics | Cycle Time, ISO 9126 Quality Score, Test Coverage |
| Borrowed From | ITIL Operations, Agile Sprinting, XP (Extreme Programming) |

### 1.2 Tactical — "What" — Doing the right things

| Aspect | Detail |
|--------|--------|
| Focus | Medium-term planning, resource allocation, process optimization |
| Activities | Sprint planning, Backlog Refinement, RAID management, Skill Matrix, setting the CMMI "Maturity Dial" per project phase |
| Metrics | Velocity, Bus Factor (replaceability), Budget, Definition of Ready compliance |
| Borrowed From | Prince2 "Managing a Stage Boundary", Scrum Master & PO roles |

### 1.3 Strategic — "Why" — Setting the direction

| Aspect | Detail |
|--------|--------|
| Focus | Long-term vision, market fit, organizational health |
| Activities | High-level ADRs, Product Vision (Sinek's Why), Portfolio management, Succession Planning, Sunset decisions |
| Metrics | ROI, Net Promoter Score (NPS), Employee Retention, Peter Principle detection |
| Borrowed From | TOGAF Architecture Vision, ITIL Service Strategy |

### The Golden Thread

Strategic intent (the ADR/Why) → visible at Tactical level (the Task/Requirement) → enforced at Operational level (the Commit/Test). Every commit traces back to a "Why."

---

## 2. The "Thin-V" Change Process

Every code change follows this collapsed V-model loop, regardless of CMMI level:

| Phase | Action | Gate | Borrowed From |
|-------|--------|------|---------------|
| **1. Justify** | Define value, identify trigger (feat/fix/chore), assign ISO 9126 quality target | Value statement exists, E2E "ghost" test drafted | Prince2 Business Case, Sinek, BDD |
| **2. Design** | Update diagrams (C4/Mermaid), define Acceptance Criteria in Given-When-Then | AC is testable, impact analysis done | V-Model, ArchiMate/C4 |
| **3. Build** | TDD: write test → write code → refactor. Follow RTFM, KISS, YAGNI, NBI, HIPI, C4C, C4I | Tests pass, lint clean, complexity ≤ 10 | Lean, XP, TDD |
| **4. Verify** | E2E tests on feature environment, mutation testing, peer review | All auto-checks green, peer sign-off | ITIL Change Enablement, ISO 9126 |
| **5. Transition** | Deploy, smoke test, close loop | Smoke tests pass, docs updated, issue closed | ITIL Service Transition, Prince2 |

---

## 3. CMMI Maturity Levels

### 3.1 Level Overview

| Level | Name | When | Philosophy |
|-------|------|------|------------|
| **CMMI 0** | Essentials | Startup / MVP / Spike | Make it work. Go fast, get feedback. |
| **CMMI 1** | Managed | First release / MUP | Make it reliable. Maintain what you ship. |
| **CMMI 2** | Defined | Stable / Production | Make it better. Measure and optimize. |
| **CMMI 3** | Optimizing | Mature / Scale | Make it excellent. Predict and prevent. |

### 3.2 CMMI 0 — Essentials (MVP)

**Goal**: Ship value fast with minimum viable quality. Gather feedback, log debt.

| # | Check | Automated | Stacking Effect | Tool |
|---|-------|-----------|-----------------|------|
| 0.1 | Conventional commits enforced | ✅ Easy | → Auto-changelog, release notes | commitlint, git hook |
| 0.2 | Branch naming convention | ✅ Easy | → Links to issue tracker, triggers CI | git hook |
| 0.3 | Basic linting (syntax, formatting) | ✅ Easy | → Suppressions become TODOs → tech debt count | clang-format, eslint |
| 0.4 | Secret detection | ✅ Easy | → Blocks commit, audit trail | gitleaks |
| 0.5 | Build compiles / passes | ✅ Easy | → Gatekeeper for all subsequent checks | make, CI |
| 0.6 | README exists with setup instructions | 🔶 Template | → Onboarding baseline, C4I compliance | pre-commit check |
| 0.7 | Manual test script documented | 📝 Manual | → Becomes E2E test input at CMMI 1 | markdown template |
| 0.8 | Value statement on task/issue | 📝 Manual | → Traces to strategic "Why" | issue template |

**Process at CMMI 0**:

1. Create branch from issue → `feat/123-short-name`
2. Code with basic lint + secret scan on pre-commit
3. Manual test, document what you tested
4. Push → CI runs build + lint + gitleaks
5. Merge after self-review (peer review optional)

### 3.3 CMMI 1 — Managed (First Release)

**Goal**: Repeatable process. What you ship, you maintain. Knowledge is documented.

Inherits all CMMI 0 checks, plus:

| # | Check | Automated | Stacking Effect | Tool |
|---|-------|-----------|-----------------|------|
| 1.1 | Unit tests required (coverage ≥ 60%) | ✅ Easy | → Coverage trend feeds quality dashboard | doctest, jest, codecov |
| 1.2 | SAST security scan | ✅ Easy | → Findings feed risk register (RAID) | semgrep, cppcheck |
| 1.3 | TODO/FIXME scraping → tech debt log | ✅ Easy | → Count = tech debt metric, feeds backlog | grep hook, custom script |
| 1.4 | Lint suppressions require reason | ✅ Medium | → `// NOLINTNEXTLINE TODO: reason` → scraped into debt log | custom lint rule |
| 1.5 | E2E test exists per feature | 🔶 Guided | → Ghost test from Justify phase now passes | playwright, bash e2e |
| 1.6 | Peer review required | ✅ Easy | → Implicit decision detection ("Is there a Why not in the ADR?") | branch protection |
| 1.7 | Definition of Ready enforced | 🔶 Template | → Tasks without AC cannot enter sprint | issue template + bot |
| 1.8 | How-to / doc updated if UI/API changed | 🔶 Medium | → Docs-as-code, KM baseline | CI check: docs/ touched |
| 1.9 | Complexity ≤ 10 per function | ✅ Easy | → Refactor trigger, feeds maintainability score | pmccabe, clang-tidy |
| 1.10 | Comment ratio ≥ 20% | ✅ Easy | → Self-documenting code metric | cloc |

**Process at CMMI 1**:

1. Task passes Definition of Ready (value statement, AC in Given-When-Then, RAID reviewed)
2. Create branch → Justify phase: draft ADR if architectural, write ghost E2E test
3. Design: update Mermaid diagrams if structure changes
4. Build: TDD cycle — test first, code, refactor. Pre-commit runs lint + complexity + tests
5. Push → CI runs full suite: build, unit tests, SAST, coverage, E2E
6. PR created → peer review after all auto-checks green
7. Merge → auto-deploy to staging, smoke test

### 3.4 CMMI 2 — Defined (Production)

**Goal**: Measured, optimized, predictable. SLAs in place. Architecture is documented and current.

Inherits all CMMI 0+1 checks, plus:

| # | Check | Automated | Stacking Effect | Tool |
|---|-------|-----------|-----------------|------|
| 2.1 | Coverage ≥ 80% | ✅ Easy | → Quality gate, trend analysis | codecov |
| 2.2 | Mutation testing | 🔶 Medium | → Test meaningfulness score | stryker, mull |
| 2.3 | C4 diagrams validated and current | 🔶 Medium | → Architecture drift detection | structurizr, mermaid-cli |
| 2.4 | AI code review (deterministic rules) | 🔶 Medium | → NBI, HIPI, C4C checks via LLM with fixed prompts | custom CI step |
| 2.5 | Performance / load baseline | 🔶 Medium | → Regression detection, ISO 9126 Efficiency | k6, benchmark suite |
| 2.6 | Accessibility compliance | 🔶 Medium | → ISO 9126 Usability, C4I | axe, pa11y |
| 2.7 | Disaster recovery plan exists | 📝 Manual | → Operational resilience | doc template |
| 2.8 | SLA monitoring active | ✅ Medium | → Incident auto-trigger if breached | uptime checks, alerting |
| 2.9 | Skill Matrix maintained | 📝 Manual | → Bus Factor metric, succession planning | SKILLS.md |
| 2.10 | Sprint retrospective with delta report | 🔶 Guided | → ISO 9126 improvement trend, lessons learned | auto-generated report |
| 2.11 | Implicit decision detection in PR | 🔶 Medium | → Scan for "chose to", "workaround" → prompt Micro-ADR | custom hook / AI |
| 2.12 | IaC for environments | 🔶 Medium | → Reproducible deploys, DR readiness | terraform, docker-compose |

**Process at CMMI 2**:

1. Sprint planning: set CMMI dial, allocate 70/20/10 (delivery/cross-training/innovation)
2. Full Thin-V per task with all CMMI 0+1+2 gates
3. Feature environments auto-provisioned per branch
4. Mutation testing runs in CI alongside unit tests
5. AI review runs deterministic checks (NBI, HIPI, complexity) with fixed prompts for idempotent results
6. Sprint ends with auto-generated delta report: coverage Δ, debt Δ, velocity, bus factor
7. Retrospective feeds lessons learned into knowledge base

### 3.5 CMMI 3 — Optimizing (Mature)

**Goal**: Predictive, self-improving. Process optimizes itself based on data.

Inherits all CMMI 0+1+2 checks, plus:

| # | Check | Automated | Stacking Effect | Tool |
|---|-------|-----------|-----------------|------|
| 3.1 | Predictive defect analysis | 🔶 Hard | → Risk-based testing focus | ML on defect history |
| 3.2 | Auto-generated release notes | ✅ Easy | → From conventional commits + ADRs | semantic-release |
| 3.3 | Cross-team dependency scanning | 🔶 Medium | → Portfolio-level risk view | custom tooling |
| 3.4 | Continuous feedback loop (DORA metrics) | ✅ Medium | → Lead time, MTTR, deploy frequency, change failure rate | CI analytics |
| 3.5 | Peter Principle detection | 🔶 Hard | → Flag when contributor scope narrows over time | bus factor trend |
| 3.6 | Auto-sunset detection | 🔶 Medium | → Flag repos with no feat commits in N months | custom script |
| 3.7 | Portfolio-level quality dashboard | 🔶 Medium | → Strategic decision support | aggregated metrics |

---

## 4. Automation Stacking Matrix

The "chess principle" — every check serves ≥ 2 purposes:

| Automation | Primary Purpose | Stacking Effect 1 | Stacking Effect 2 |
|------------|----------------|--------------------|--------------------|
| Lint suppression → TODO comment | Code quality | Tech debt count metric | Feeds sprint backlog |
| Conventional commits | Commit hygiene | Auto-changelog | Release note generation |
| Branch naming `type/id-name` | Traceability | Issue linking | CI trigger routing |
| Coverage tracking | Test quality | Quality trend dashboard | Refactor priority signal |
| TODO scraping | Debt visibility | Debt count per module | Sprint planning input |
| Complexity check | Maintainability | Refactor trigger | Skill gap indicator |
| Secret detection | Security | Audit trail | Compliance evidence |
| E2E ghost test | BDD alignment | Regression suite growth | Feature documentation |
| Peer review gate | Knowledge sharing | Implicit decision catch | Bus factor improvement |
| C4 diagram validation | Architecture currency | Onboarding material | Drift detection |
| AI deterministic review | Consistency | NBI/HIPI/C4C enforcement | Learning feedback for dev |
| Mutation testing | Test meaningfulness | False confidence detection | Coverage quality metric |
| Sprint delta report | Process visibility | Retrospective input | Strategic trend data |

---

## 5. Git-Triggered Process Flow

```text
Issue "In Progress"
  │
  ├─► Branch created: feat/123-short-name
  │     └─► CMMI level inherited from project config
  │
  ├─► PRE-COMMIT HOOKS (per CMMI level)
  │     ├─ CMMI 0: format, lint, secrets
  │     ├─ CMMI 1: + tests, complexity, TODO scrape
  │     ├─ CMMI 2: + diagram check, AI review
  │     └─ CMMI 3: + cross-dependency scan
  │
  ├─► PUSH → CI PIPELINE
  │     ├─ Build + unit tests
  │     ├─ SAST + coverage
  │     ├─ E2E on feature environment (CMMI 1+)
  │     ├─ Mutation testing (CMMI 2+)
  │     └─ Performance baseline (CMMI 2+)
  │
  ├─► PR CREATED (draft until CI green)
  │     ├─ Auto-checks summary posted
  │     ├─ Peer review required (CMMI 1+)
  │     └─ Implicit decision scan (CMMI 2+)
  │
  ├─► MERGE → main
  │     ├─ Auto-deploy to staging/production
  │     ├─ Smoke tests
  │     ├─ Issue auto-closed
  │     └─ Tech debt log updated
  │
  └─► POST-RELEASE
        ├─ Monitoring / SLA check (CMMI 2+)
        └─ Incident auto-trigger if error spike
```

---

## 6. Pre-Sprint Funnel

### 6.1 Discovery (Creating Tasks)

| Gate | Check | Automation |
|------|-------|------------|
| Value statement | "Why" exists (Sinek) | Template enforcement |
| ISO 9126 target | Quality attribute identified | Required field |
| Testability | AC in Given-When-Then format | Format validation |

### 6.2 Refinement (Preparing Tasks)

| Gate | Check | Automation |
|------|-------|------------|
| RAID reviewed | Risks, Assumptions, Issues, Dependencies logged | Checklist in template |
| Dependencies mapped | Blocked tasks identified | Dependency graph scan |
| Acceptance Criteria signed off | Peer-reviewed AC | Review flag |
| Estimate provided | Effort sized | Required field |

### 6.3 Sprint Planning

| Gate | Check | Automation |
|------|-------|------------|
| Definition of Ready met | All Discovery + Refinement gates pass | Bot validation |
| CMMI level set for sprint | Maturity dial configured | Project config |
| Work mix balanced | feat/fix/chore ratio visible | Dashboard |
| Capacity allocated | 70% delivery / 20% cross-training / 10% innovation | Planning template |

---

## 7. Work Type Routing

| Type | Trigger | Priority | Process Path |
|------|---------|----------|--------------|
| **feat** (Change) | Sprint backlog | Planned | Full Thin-V: Justify → Design → Build → Verify → Transition |
| **fix** (Incident) | Ad-hoc (monitoring/users) | Critical | Fast-track: skip Refinement, straight to Build → Verify |
| **chore/refactor** (Problem) | Tech debt log | Scheduled | Lean Thin-V: skip ADR if minor, focus on Build → Verify |
| **spike** (Innovation) | Learning need | Time-boxed | Sandbox: no guardrails, code is disposable, output = knowledge |

---

## 8. Product Lifecycle Phases

| Phase | CMMI Level | Allowed Work | Deploy Strategy | Key Activity |
|-------|------------|--------------|-----------------|--------------|
| **MVP** | 0 | All types | Manual / basic CI | Ship fast, gather feedback, log debt |
| **First Release** | 1 | All types | Automated CI/CD | Stabilize, document, automate tests |
| **Production** | 2 | All types | IaC, feature envs | Optimize, measure, SLA |
| **Legacy** | 1 (locked) | fix + chore only | Patch pipeline | Security fixes, dependency updates only |
| **Sunset** | 0 (locked) | chore only | Archive script | Take offline, check for accidental dependents |

---

## 9. Roles (Team Topologies)

| Role | V-Model Alignment | Primary Responsibility | Pillar |
|------|-------------------|----------------------|--------|
| **Product Owner** | Business Req / UAT | Value definition, "The Why", guards the Business Case | Strategic + Tactical |
| **Scrum Master** | Process / RAID | Impediment removal, ritual facilitation, process health | Tactical |
| **Dev (FE/BE)** | Design / Unit / Integration | Implementation, RTFM, HIPI, T-shaped skills | Operational |
| **QA (Enabling)** | Acceptance / System Test | Test framework building, quality assurance (not just testing) | Operational + Tactical |
| **Platform (Foundation)** | Environment / Deploy | Automating the V-climb, feature environments, IaC | Operational |

### Succession & Growth

- **Bus Factor**: automation tracks unique contributors per module. Single-contributor modules are flagged as risk.
- **Shadow Rotation**: every 3rd sprint, devs shadow a different role (BE→DevOps, FE→QA).
- **Dual-Ladder Growth**: depth (Staff Engineer) or breadth (Architect/Lead) — no forced management track.
- **70/20/10 Rule**: 70% delivery, 20% cross-training, 10% innovation/spikes.

---

## 10. The Seven Coding Principles

Enforced progressively through automation:

| Principle | Meaning | Enforcement Level | Automation |
|-----------|---------|-------------------|------------|
| **RTFM** | Respect The Framework's Model | CMMI 0+ | Link to docs on lint failure |
| **C4C** | Coding For Clarity | CMMI 1+ | Complexity check, comment ratio |
| **C4I** | Code for Inclusivity | CMMI 1+ | Onboarding test: can a new dev run it? |
| **KISS** | Keep It Simple Stupid | CMMI 1+ | Complexity ≤ 10, LOC per PR flag |
| **YAGNI** | You Aren't Gonna Need It | CMMI 1+ | PR scope check vs. task AC |
| **HIPI** | Hide Implementation, Present Interface | CMMI 2+ | AI review: public API surface check |
| **NBI** | Naming by Intention | CMMI 2+ | AI review: flag generic names (data, temp, handle) |

---

## 11. Knowledge Management

| Artifact | When Updated | Automation |
|----------|-------------|------------|
| ADR | Architectural decisions | Template on branch create (feat) |
| Micro-ADR | Implicit decisions caught in review | Prompted by comment scan |
| How-to | UI/API changes | CI fails if docs/ not touched |
| TECHDEBT.md | Every sprint | Auto-scraped from TODOs |
| SKILLS.md | Sprint retrospective | Manual (CMMI 2+) |
| C4 Diagrams | Design phase | Validated in CI (CMMI 2+) |
| Lessons Learned | Sprint close | Auto-archived from RAID |
| Delta Report | Sprint close | Auto-generated: coverage Δ, debt Δ, velocity |

**KT Rule**: if a dev can't go on holiday without being called, the documentation has a bug. Replaceability is a Definition of Done item.

---

## 12. AI Review Guidelines

To ensure **consistent, idempotent results**, AI reviews use:

1. **Fixed prompts** — no creative/open-ended review. Checklist-based.
2. **Deterministic rules** — check NBI, HIPI, C4C, complexity, TODO format.
3. **Structured output** — pass/fail per rule, with file:line references.
4. **No subjective feedback** — "this could be cleaner" is not actionable. "Function `processData` violates NBI: name doesn't express purpose" is.
5. **Idempotent** — same code → same review output. No randomness.

---

## 12a. Model Selection: Right Tool for the Right Job

The framework is designed so that **any AI agent can execute tasks** — but matching model capability to task complexity makes the difference between success and frustration. See [docs/model-guide.md](../model-guide.md) for the full guide with personas and community ratings.

### The Cast (Quick Reference)

| Tool | Persona | Best For |
|------|---------|----------|
| **kiro-cli** (Claude 4 Sonnet) | "The Senior Architect" | Design decisions, complex refactors, 8+ SP tasks |
| **Gemini CLI** (Gemini 2.5 Pro) | "The Eager Mid-Level" | General coding, 3-5 SP tasks, 1000 req/day free |
| **tgpt** (Pollinations/FreeGpt) | "The Unreliable Intern" | Spikes, quick questions, disposable code |
| **gemma4:e4b** (Ollama, 9.6GB) | "The Reliable Junior" | Prompt execution (docs/prompts/), 1-2 SP tasks |
| **gemma4:26b** (Ollama, 17GB) | "The Thoughtful Mid-Level" | Tests, moderate tasks, best local option |

### Task-to-Model by Story Points

| SP | Complexity | Model | Example |
|----|-----------|-------|---------|
| 1-2 | Trivial/Simple | gemma4:e4b + prompt | Git hooks, config files, threshold bumps |
| 3 | Moderate | gemma4:26b or Gemini CLI | CI jobs, script modifications, unit tests |
| 5 | Complex | Gemini CLI or kiro-cli | New feature (API layer), module refactor |
| 8+ | Hard/Epic | kiro-cli + human review | Multi-module wiring, architecture changes |

### By CMMI Level

| Level | Minimum Model | Why |
|-------|--------------|-----|
| CMMI 0 | gemma4:e4b | All tasks have copy-paste prompts in `docs/prompts/` |
| CMMI 1 | gemma4:26b or Gemini CLI | Some tasks need reasoning about existing code |
| CMMI 2+ | Gemini CLI or kiro-cli | AI review, architecture validation, optimization |

---

## 13. Audit Checklist (Self-Assessment)

Use this to determine your current CMMI level:

### CMMI 0 — Essentials

- [ ] Conventional commits enforced
- [ ] Branch naming convention in place
- [ ] Basic linting runs on commit
- [ ] Secret detection active
- [ ] Build compiles in CI
- [ ] README with setup instructions exists

### CMMI 1 — Managed

- [ ] All CMMI 0 checks pass
- [ ] Unit test coverage ≥ 60%
- [ ] SAST scan in CI
- [ ] TODO/FIXME scraping active
- [ ] Peer review required for merge
- [ ] Definition of Ready enforced
- [ ] E2E tests exist per feature
- [ ] Docs updated when API/UI changes
- [ ] Complexity limit enforced (≤ 10)

### CMMI 2 — Defined

- [ ] All CMMI 1 checks pass
- [ ] Coverage ≥ 80%
- [ ] Mutation testing in CI
- [ ] C4 diagrams validated
- [ ] AI deterministic review active
- [ ] Performance baseline tracked
- [ ] SLA monitoring in place
- [ ] Skill Matrix maintained
- [ ] IaC for all environments
- [ ] Sprint delta reports generated

### CMMI 3 — Optimizing

- [ ] All CMMI 2 checks pass
- [ ] DORA metrics tracked (lead time, MTTR, deploy freq, change failure rate)
- [ ] Predictive defect analysis active
- [ ] Cross-team dependency scanning
- [ ] Portfolio-level quality dashboard
- [ ] Auto-sunset detection for dormant repos

---

## 14. Defence: Why This Framework

### The 5× Why Test

Every element in this framework must survive the "5× Why" drill. If you can't answer five levels deep, the element doesn't belong. Here's how the framework holds up:

**Example 1: "Why do we enforce conventional commits?"**

| Level | Question | Answer |
|-------|----------|--------|
| Why 1 | Why conventional commits? | So every commit has a clear type (feat/fix/chore) |
| Why 2 | Why does the type matter? | So we can auto-generate changelogs and route CI checks |
| Why 3 | Why auto-generate changelogs? | So users know what changed without manual effort (lean) |
| Why 4 | Why does the user need to know? | So they can assess upgrade risk (ITIL Change Enablement) |
| Why 5 | Why assess risk? | Because the Strategic "Why" (product trust) depends on transparent change history |

**Example 2: "Why do lint suppressions become TODO comments?"**

| Level | Question | Answer |
|-------|----------|--------|
| Why 1 | Why force a TODO on suppression? | So the suppression is visible as tech debt |
| Why 2 | Why track tech debt? | So it feeds the sprint backlog (Tactical planning) |
| Why 3 | Why does the backlog need it? | So we can prioritize debt vs. features (informed decisions) |
| Why 4 | Why prioritize debt? | Because unchecked debt slows delivery (Lean waste) |
| Why 5 | Why does delivery speed matter? | Because the Strategic "Why" (fast feedback, market fit) depends on sustainable velocity |

### The Black-Box Principle: Design Top-Down, Build Bottom-Up

Think of the framework as nested boxes. From the outside, you see a label. The label links to what's inside. What's inside can be verified. Inside that, there may be another box.

```text
┌─────────────────────────────────────────────────────────┐
│ STRATEGIC: "Why are we building this?"                  │
│ Label: Product Vision, ADRs                             │
│ Verify: Does the ADR exist? Does it state the Why?      │
│                                                         │
│  ┌───────────────────────────────────────────────────┐  │
│  │ TACTICAL: "What should we build this sprint?"     │  │
│  │ Label: Sprint Backlog, RAID, DoR                  │  │
│  │ Verify: Does the task have AC? Is RAID reviewed?  │  │
│  │                                                   │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │ OPERATIONAL: "How do we build it right?"    │  │  │
│  │  │ Label: Commit, Test, Review                 │  │  │
│  │  │ Verify: Does CI pass? Is coverage met?      │  │  │
│  │  │                                             │  │  │
│  │  │  ┌───────────────────────────────────────┐  │  │  │
│  │  │  │ THE CODE: The actual change           │  │  │  │
│  │  │  │ Label: feat/fix/chore + issue ID      │  │  │  │
│  │  │  │ Verify: Tests green, lint clean       │  │  │  │
│  │  │  └───────────────────────────────────────┘  │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

Every box is:

- **Labelled** — you know what it is without opening it
- **Linked** — the label references what's inside (ADR → task → commit → test)
- **Verifiable** — you can check it from outside (CI status, checklist, metric)
- **Nestable** — boxes contain boxes, each with the same properties

### Why This Framework Works

| Claim | How | Evidence |
|-------|-----|----------|
| **Lean** | Every check serves ≥ 2 purposes (chess principle). No ceremony without output. | Section 4: Automation Stacking Matrix — 13 automations, each with 2+ effects |
| **Efficient** | Checks run at the right time: fast checks early (pre-commit), heavy checks late (CI). No wasted cycles. | Section 5: Git-Triggered Process Flow — hooks are tiered by CMMI level |
| **Effective** | Catches real problems: secrets before commit, complexity before review, regressions before merge. | Section 3: CMMI levels — each check maps to a concrete quality outcome |
| **Grows with need** | CMMI 0→3 is a dial, not a cliff. Start with 8 checks, grow to 37. Add checks when the product phase demands it. | Section 8: Product Lifecycle Phases — MVP→Production→Legacy |
| **Clear for everyone** | A PO reads the Strategic box. A dev reads the Operational box. Same framework, different zoom level. | Section 9: Roles — each role maps to a pillar and V-Model level |
| **Accessible** | No jargon without definition. Every acronym is explained. Principles have plain-English names (KISS, YAGNI). | Section 10: Seven Coding Principles — each has a one-line meaning |
| **Tailorable** | CMMI level is per-project, per-sprint. Checks can be disabled with documented reason (→ TODO → debt log). | Section 3.2-3.5: each level inherits and extends, never replaces |
| **Traceable** | Every commit → task → ADR → product vision. The Golden Thread is not a metaphor, it's enforced by branch naming + conventional commits + issue linking. | Section 1: The Golden Thread |

### The "If Only I Had This Sooner" Test

The framework passes this test because:

1. **It doesn't ask you to stop and set up governance.** You start at CMMI 0 with 8 checks that take an afternoon to implement. You're already shipping.
2. **It doesn't punish you for moving fast.** Spikes have no guardrails. Suppressions are allowed — they just become visible debt.
3. **It doesn't require a specific team size.** Solo dev? CMMI 0-1. Team of 5? CMMI 1-2. Enterprise? CMMI 2-3. Same framework.
4. **It doesn't require specific tools.** The checks are tool-agnostic. "Secret detection" can be gitleaks, trufflehog, or a custom script.
5. **It makes quality visible, not invisible.** You always know your CMMI level. The audit checklist (Section 13) is a 2-minute self-assessment.

---

## 15. Current Status & Audit (2026-04-18)

### Project: llama-cli

**Current CMMI Level: 0.6** (5 of 8 Essentials pass)

### CMMI 0 — Essentials Audit

| # | Check | Status | Evidence | Gap |
|---|-------|--------|----------|-----|
| 0.1 | Conventional commits | ❌ FAIL | No commitlint, no git hook, no CI check for commit format | Need: commit-msg hook or CI job |
| 0.2 | Branch naming convention | ❌ FAIL | `pre-commit.sh` blocks `main` but no pattern validation | Need: regex check in pre-commit |
| 0.3 | Basic linting | ✅ PASS | `.config/.clang-format` (Google style), CI `lint-cpp` job, pre-commit auto-formats | — |
| 0.4 | Secret detection | ✅ PASS | CI `sast-secret` job (gitleaks), pre-commit runs `sast-secret` | — |
| 0.5 | Build compiles | ✅ PASS | CI `build` job runs `make build` | — |
| 0.6 | README with setup | ✅ PASS | Install (binary + source), Usage, Configuration sections | — |
| 0.7 | Test scripts documented | ✅ PASS | `make test`, `make e2e`, 5 E2E scripts in `e2e/` | — |
| 0.8 | Issue/PR templates | ❌ FAIL | No `.github/ISSUE_TEMPLATE/` or `PULL_REQUEST_TEMPLATE.md` | Need: templates with value statement + AC |

### CMMI 1 — Managed Audit (preview)

| # | Check | Status | Evidence | Gap |
|---|-------|--------|----------|-----|
| 1.1 | Coverage ≥ 60% | ❌ FAIL | CI threshold is 55% (`ci.yml:217`) | Need: bump to 60%, add tests |
| 1.2 | SAST security | ✅ PASS | CI `sast-security` job (semgrep) | — |
| 1.3 | TODO scraping → debt log | ❌ FAIL | `make todo` displays but doesn't write to file | Need: pipe to `TECHDEBT.md` |
| 1.4 | Suppressions require reason | ✅ PASS | `complexity.sh` requires `pmccabe:skip-complexity` annotation | — |
| 1.5 | E2E per feature | ✅ PASS | 5 E2E scripts: live, repl, files, sync, smart-rw | — |
| 1.6 | Peer review required | ❌ FAIL | CI runs on PR but no branch protection rules | Need: GitHub branch protection |
| 1.7 | Definition of Ready | ❌ FAIL | No issue templates | Need: issue template with DoR checklist |
| 1.8 | Doc-change enforcement | ❌ FAIL | No CI check for docs/ when src/ changes | Need: CI job |
| 1.9 | Complexity ≤ 10 | ✅ PASS | `make complexity` via pmccabe, skip annotation supported | — |
| 1.10 | Comment ratio ≥ 20% | ✅ PASS | `scripts/check/comment-ratio.sh` with `THRESHOLD=20` | — |

### What's Already Strong (hard to retrofit, already done)

| Capability | Evidence |
|------------|----------|
| 16 CI jobs | `ci.yml`: changes, version-bump, 8× lint, build, unit-test, e2e-test, coverage, sanitizers, 2× SAST, comment-ratio |
| Pre-commit pipeline | `precommit-check.sh`: 6 checks (format-cpp, format-yaml, format-markdown, format-scripts, index, sast-secret) |
| Architecture docs | `docs/architecture.md`, `docs/architecture-v2.md` with C4/Mermaid diagrams |
| AI code review | CodeRabbit integration (`scripts/gh/pr-feedback.sh`) |
| 48 ADRs | Comprehensive decision log covering all major choices |
| Modular source | 15 modules in `src/`, co-located tests (`*_test.cpp`, `*_it.cpp`) |
| Sanitizers | ASan + UBSan in CI (`sanitizers` job) |
| Low tech debt | Only 3 TODO markers in entire source |

### Sprint 1 Plan — Reach CMMI 0

Migrated to backlog. See:

| Backlog | Task | CMMI |
|---------|------|------|
| [023](../backlog/023-commit-msg-hook.md) | Commit message validation | 0.1 |
| [024](../backlog/024-branch-naming.md) | Branch naming validation | 0.2 |
| [025](../backlog/025-issue-pr-templates.md) | Issue/PR templates | 0.8 |
| [026](../backlog/026-changelog.md) | CHANGELOG.md | 0 |

### Sprint 2 Plan — Reach CMMI 1 + Streaming

Migrated to backlog. See:

| Backlog | Task | CMMI |
|---------|------|------|
| [027](../backlog/027-coverage-bump.md) | Coverage bump 55%→60% | 1.1 |
| [028](../backlog/028-todo-scraping.md) | TODO scraping → TECHDEBT.md | 1.3 |
| [029](../backlog/029-branch-protection.md) | Branch protection (peer review) | 1.6 |
| [030](../backlog/030-doc-change-ci.md) | Doc-change CI enforcement | 1.8 |
| [005](../backlog/005-streaming.md) | Streaming responses | Feature |

AI agent prompts for these tasks live in [`docs/prompts/`](../prompts/).

### AI Agent Gap Analysis

**Question**: Can a "dumb" local model (e.g., gemma4:e4b via Ollama) execute these tasks?

**Answer**: Yes, IF each task is provided as a self-contained prompt with:

| Requirement | Why | How we solve it |
|-------------|-----|-----------------|
| Exact file paths | Model can't search the codebase | Every prompt specifies full paths |
| Exact code to write | Model can't infer project conventions | Every prompt has copy-paste code blocks |
| Exact "find and replace" anchors | Model can't guess what to change | Prompts use literal text to find |
| Verification commands | Model can't judge its own work | Every prompt has `bash` commands with expected output |
| Commit message | Model can't infer conventional format | Every prompt ends with the exact commit message |
| Sequential ordering | Model can't resolve dependencies | Prompts are numbered; dependencies are stated |

**Identified gaps in the original ticket format** (remediated):

| Gap | Risk | Remediation |
|-----|------|-------------|
| Tickets described WHAT, not exact code | Model writes wrong code, breaks build | Created `docs/prompts/` with copy-paste code |
| No verification commands | Model can't confirm success | Every prompt has verify + expected output |
| Streaming was one big task | Too complex for a small model | Split into 3 sub-tasks (API → REPL → tests) |
| Coverage bump had no specific target | Model doesn't know which line to change | Prompt specifies exact line to find/replace |
| Branch protection is a GitHub API call | Model might try to write code | Prompt gives exact `gh` CLI command |
| TODO scraping needed script surgery | Model might rewrite the whole script | Prompt shows exact function replacement |

**Prompt format** (standardized in `docs/prompts/`):

```text
# Prompt NN: Title
## Context     — what exists, what it looks like
## Task        — what to do
## Code        — exact file content or find/replace
## Verify      — bash commands to confirm
## Expected    — what the output should look like
## Commit      — the conventional commit message
```

### Path to CMMI 2 (when entering Production phase)

Coverage 80%, mutation testing, performance baselines, IaC, SLA monitoring, skill matrix.

---

## Alternatives Considered

| Approach | Decision | Reason |
|----------|----------|--------|
| Full CMMI 5-level model | **Adapted to 4 levels (0-3)** | CMMI 4-5 are statistically managed; overkill for most teams. Levels 0-3 cover the practical range. |
| Separate quality framework doc | **Embedded in ADR** | ADRs are the decision record. The framework IS the decision. |
| Subjective AI reviews | **Deterministic only** | Subjective reviews produce inconsistent results. Fixed prompts + structured output = idempotent. |
| Individual performance tracking | **Team-level metrics only** | Privacy-safe. Bus Factor and DORA metrics measure the system, not the person. Skill Matrix is opt-in. |
| Heavyweight ITIL processes | **Lean adaptation** | Full ITIL is designed for enterprise IT ops. We borrow the concepts (Change/Incident/Problem) but collapse the ceremony. |

## References

| Source | Borrowed Concept |
|--------|-----------------|
| ITIL 4 | Change/Incident/Problem management, Service Strategy, CSI |
| Prince2 | Business Case, Managing a Stage Boundary, Closing a Project |
| V-Model | Level-matched validation, top-down design / bottom-up test |
| Agile/Scrum | Sprints, DoR, DoD, Retrospectives, Backlog Refinement |
| CMMI | Maturity levels, process areas, continuous improvement |
| ISO 9126 | Quality characteristics (Functionality, Reliability, Usability, Efficiency, Maintainability, Portability) |
| TOGAF | Preliminary Phase (bootstrap), Architecture Vision |
| Lean/Kaizen | Waste reduction, continuous improvement, Genba |
| XP | TDD, pair programming, collective ownership |
| Team Topologies | Stream-aligned, Enabling, Platform teams |
| Sinek (Start with Why) | Value justification, Golden Circle |
| BDD | Given-When-Then, behavior-driven acceptance criteria |
| DORA/Accelerate | Four key metrics for software delivery performance |
| Peter Senge | Learning Organization |
| Laurence J. Peter | Peter Principle detection and avoidance |
| David Guest | T-shaped skills |
