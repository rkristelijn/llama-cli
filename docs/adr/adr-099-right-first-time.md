---
summary: The analysis of 465 commits reveals high tooling rework, but the current status and health targets need improvement, with a goal of reducing tooling rework to <20% and bug fix ratio to <40%.
title: "ADR-099: Right the First Time — Tooling ROI and Shift-Left Strategy"
status: accepted
date: 2026-05-07
---

summary: The analysis of 465 commits reveals high tooling rework, but the current status and health targets need improvement, with a goal of reducing tooling rework to <20% and bug fix ratio to <40%.

## ADR-099: Right the First Time — Tooling ROI and Shift-Left Strategy

### Context

Analysis of 465 commits (Jan–May 2026) reveals:

- **35% of all commits are fixes** (163/465)
- **37% of fixes are tooling rework** (61/163) — fixing CI, lint, format, SAST tools
- **46% of productive commits** (feat+bugfix) are bug fixes
- macOS build failures were only caught at release time (not in PR pipeline)

The tooling itself generates rework. Every new check added to the pipeline has a "fix the check" tax that can exceed the bugs it catches.

### Decision

#### 1. Tooling ROI Rule

Every automated check must satisfy: **bugs caught > fix commits generated**.

Before adding a new check, answer:

- How many bugs would this have caught in the last 50 commits?
- How many fix commits will this generate to get it working?

If the ratio is <2:1, don't add it. Prefer checks that auto-fix (`make format`) over checks that only report.

#### 2. Platform Parity in CI

All release targets must compile in the PR pipeline. Added `build-macos` job to `ci.yml` (macos-14 runner). This prevents the class of bugs where code compiles on Linux but fails on macOS (e.g., `Style` typedef conflict with `MacTypes.h`).

#### 3. Scope Enforcement (recommended, not yet enforced)

Commit scopes should map to known areas to enable reliable tracking:

| Category | Scopes |
|----------|--------|
| Features | `repl`, `config`, `ollama`, `tui`, `provider`, `agent`, `exec`, `sync`, `tui` |
| Infra | `ci`, `lint`, `test`, `build`, `security` |
| Meta | `docs`, `git`, `hooks`, `dev` |

Enforcement via `commit-msg.sh` is optional — start by tracking with `make commit-stats`.

#### 4. Health Targets

| Metric | Current | Target |
|--------|---------|--------|
| Tooling rework (% of fixes) | 37% | <20% |
| Bug fix ratio (% of productive) | 46% | <40% |
| Platform coverage in CI | 1/3 | 3/3 |

Track with `make commit-stats --since <last-release-tag>`.

### Consequences

- New checks require justification (ROI)
- macOS failures caught at PR time, not release time
- `make commit-stats` provides repeatable health reporting
- Scope tracking enables data-driven decisions about where to invest quality effort
- Reduces "fix the fixer" loops that consume 37% of fix effort

### Alternatives Considered

- **Enforce scopes now**: Rejected — too disruptive. Start with tracking, enforce later if data supports it.
- **Remove low-ROI checks**: Deferred — need baseline data from `commit-stats` over 2-3 sprints first.
- **Cross-compile for macOS on Linux**: Not practical without macOS SDK headers.

## Infrastructure & Delegation Workflow

### Available Compute

| Device | CPU | GPU | RAM | Role |
|--------|-----|-----|-----|------|
| MacBook M2 | Apple Silicon | Unified 16-core | 32GB | Primary worker — big models (14B-27B) |
| Intel MacBook (Linux) | i7 | None | 16GB | Dev machine — Kiro CLI, orchestration |
| Intel NUC ×2 | i5 | None | 16GB | Fast small models (3B-4B), background tasks |

All devices run Ollama and are reachable via hostname on the local network.

### Delegation Model (Agile Team)

```text
┌─────────────────────────────────────────────────────┐
│  Manager / Analyst (Kiro CLI on dev machine)        │
│  • Understands the big picture, designs approach    │
│  • Delegates by ROLE, not by model/host             │
│  • Reviews output, integrates, runs quality gates   │
├─────────────────────────────────────────────────────┤
│  Router (.config/delegation.local.yml)              │
│  • Maps role → model@host (gitignored)              │
│  • Knows which hardware suits which task            │
│  • Can be updated without code changes              │
├─────────────────────────────────────────────────────┤
│  Workers (Ollama on network devices)                │
│  • Receive clear task + acceptance criteria         │
│  • Run async — don't block the manager             │
│  • Results collected via /tasks command             │
└─────────────────────────────────────────────────────┘
```

### Role Types

| Role | Use case | Ideal hardware |
|------|----------|----------------|
| `code` | Write code, implement features | GPU device, 14B+ model |
| `research` | Explain, analyze, compare options | GPU device, 26B+ model |
| `review` | Check quality, find issues | Any device, 12B+ model |
| `fast` | Simple lookups, formatting | NUC, 3-4B model |
| `test` | Generate test cases | GPU device, 14B+ model |
| `docs` | Write documentation | GPU device, 26B+ model |

### Startup

Devices run Ollama as a service. From any machine on the network:

```bash
ssh <device> ollama serve   # if not already running
```

No special setup needed — delegation uses standard Ollama HTTP API on port 11434.
