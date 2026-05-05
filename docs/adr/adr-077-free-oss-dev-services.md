# ADR-077: Free Open-Source Development Services Strategy

*Status*: Accepted

## Date

2026-05-05

## Context

As a solo developer maintaining an open-source C++ project, we need to maximize quality without budget. Many SaaS tools offer free tiers for public repositories. This ADR documents which services we use, which we should add, and how they complement our local tooling.

## Decision

### Currently active (free for open source)

| Service | Purpose | Integration | Value |
|---------|---------|-------------|-------|
| **GitHub Actions** | CI/CD | `.github/workflows/` | 2000 min/month free; runs `make check` |
| **SonarCloud** | Deep static analysis (C++, shell) | GitHub App + `make sonar-report` | Catches issues local tools miss (S912, S1912) |
| **CodeRabbit** | AI PR review | GitHub App (auto) | Catches dead code, security gaps, style issues |
| **Codecov** | Coverage tracking + PR decoration | GitHub Action | Unlimited for public repos; trend tracking |
| **Dependabot** | Dependency updates | `.github/dependabot.yml` | Built into GitHub, free for all repos |

### Recommended additions (free for open source)

| Service | Purpose | Effort | Priority |
|---------|---------|--------|----------|
| **OpenSSF Scorecard** | Security posture score (badge) | Add GitHub Action | High — credibility signal |
| **DeepSource** | Static analysis + auto-fix PRs | GitHub App install | Medium — overlaps SonarCloud but adds auto-fix |
| **FOSSA** | License compliance (5 projects free) | GitHub App | Low — useful if adding dependencies |
| **Renovate** | Dependency updates (more flexible than Dependabot) | GitHub App or self-hosted | Low — Dependabot already covers this |
| **Step Security Harden-Runner** | CI supply-chain protection | Add to workflows | Medium — monitors network egress in CI |
| **Snyk** | Vulnerability scanning (limited free) | GitHub App | Low — overlaps with osv-scanner/grype |

### Not recommended (overlap or low value)

| Service | Reason |
|---------|--------|
| **Codacy** | Overlaps SonarCloud; less C++ support |
| **Code Climate** | No C++ support |
| **CircleCI** | GitHub Actions already sufficient |
| **CodeFactor** | Shallow analysis; SonarCloud is deeper |

### Strategy: layered quality with zero cost

```
Local (instant)          → make check (clang-tidy, cppcheck, shellcheck, etc.)
PR gate (minutes)        → GitHub Actions CI + CodeRabbit AI review
Cloud analysis (async)   → SonarCloud deep scan + Codecov trends
Supply chain (passive)   → Dependabot + OpenSSF Scorecard
```

### How to maximize value from each tool

**CodeRabbit** — Already integrated. Use `make gpf` to pull feedback locally. Fix "Major" and "Potential issue" items; skip "Low value" nitpicks unless trivial.

**SonarCloud** — Use `make sonar-report` for CLI summary. Focus on BLOCKERs and VULNERABILITYs. Code smells in tui/ are known (ADR-074) and tracked for refactoring.

**Codecov** — Already integrated. Enforces coverage doesn't regress on PRs. No action needed.

**OpenSSF Scorecard** (to add) — Provides a public score (0-10) on security practices. Checks: branch protection, CI tests, dependency updates, SAST, signed releases, etc. Adding the badge signals project maturity.

### Implementation plan

1. ✅ SonarCloud — active, `make sonar-report` works
2. ✅ CodeRabbit — active, `make gpf` pulls feedback
3. ✅ Codecov — active via CI
4. ✅ Dependabot — active
5. ⬚ OpenSSF Scorecard — add `scorecard.yml` workflow + badge
6. ⬚ Step Security Harden-Runner — add to CI workflows

## Consequences

- Zero monthly cost for all quality tooling
- Multiple independent analysis engines catch different issue classes
- Solo developer gets team-level review quality via AI (CodeRabbit) and deep analysis (SonarCloud)
- OpenSSF Scorecard badge communicates security posture to users
- Local `make check` remains the primary gate; cloud tools are additive

## References

- @see ADR-074 (SonarCloud integration)
- @see ADR-002 (quality checks & CI pipeline)
- @see ADR-048 (quality framework)
- @see `scripts/gh/pr-feedback.sh` — CodeRabbit feedback script
- @see `scripts/security/sonar-report.sh` — SonarCloud CLI report
