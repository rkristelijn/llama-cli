# ADR-077: Free Open-Source Development Services Strategy

*Status*: Proposed · *Date*: 2026-05-06

## Context

We use several free-for-OSS services (Codecov, SonarCloud, CodeRabbit) but haven't systematically evaluated what's available. Adding more tools increases quality visibility without cost.

## Decision

### Currently Active

| Service | Purpose | Integration |
|---------|---------|-------------|
| Codecov | Test coverage tracking + patch coverage | GitHub App |
| SonarCloud | Code quality, bugs, security hotspots | GitHub App |
| CodeRabbit | AI-powered PR review | GitHub App |
| Gitleaks | Secret scanning | CI job |
| Semgrep | SAST security rules | CI job |
| Trivy | IaC + vulnerability scanning | CI job |
| Grype | Container/binary vulnerability | CI job |

### Planned Additions (Priority Order)

| # | Service | Purpose | Effort | Value |
|---|---------|---------|--------|-------|
| 1 | **Renovate** | Auto-update pinned action versions + deps | Add `renovate.json` | High — eliminates manual version bumps |
| 2 | **OpenSSF Scorecard** | Security posture badge + recommendations | GitHub Action | High — credibility + finds gaps |
| 3 | **Socket.dev** | Supply chain attack detection | GitHub App install | Medium — protects against typosquatting |
| 4 | **FOSSA** | License compliance scanning | GitHub App install | Medium — ensures OSS license compat |
| 5 | **Deepsource** | Code quality trends + anti-patterns | GitHub App install | Low — overlaps with SonarCloud |
| 6 | **Codacy** | Quality metrics over time (trends) | GitHub App install | Low — overlaps with SonarCloud |
| 7 | **Step Security Harden-Runner** | CI exfiltration detection | 1 line per workflow | Low — defense in depth |

### Selection Criteria

- **Free for public repos** — no paid tier required
- **Zero maintenance** — GitHub App or single config file
- **Non-blocking** — informational checks, not required for merge
- **No overlap** — each tool covers a unique gap

### Not Selected

| Tool | Reason |
|------|--------|
| Snyk | Overlaps with Grype + Trivy for our use case (no npm/pip deps) |
| Dependabot | Renovate is more configurable and handles non-GitHub sources |
| CodeClimate | Free tier too limited, SonarCloud covers same ground |

## Consequences

- More visibility into project health without manual effort
- Badges on README increase contributor confidence
- Renovate eliminates stale pinned versions (currently manual `make update`)
- OpenSSF Scorecard provides actionable security improvements
- No additional CI minutes cost (GitHub Apps run on their own infra)
