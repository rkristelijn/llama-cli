# ADR-002: Quality Checks & CI Pipeline

## Status
Accepted

## Date
2026-04-10

## Context
This is a public repo meant to demonstrate structure and discipline — a "code-flex" of maintainability. The codebase must stay manageable, secure, and flexible as it grows.

## Decision
We enforce the following automated quality gates on every PR:

| Tool | Purpose | Why |
|------|---------|-----|
| **cppcheck** | Static C++ analysis | Catches bugs, style issues, and undefined behavior before runtime |
| **semgrep** | SAST security scanning | Detects insecure patterns and common vulnerabilities |
| **gitleaks** | Secret detection | Prevents accidental credential leaks in a public repo |
| **cloc (20% comment ratio)** | Documentation enforcement | Forces code to be self-documenting and readable |
| **pre-commit hook** | Branch protection | Blocks direct commits to main, enforces feature branch workflow |
| **VERSION file** | Semver tracking | CI verifies version bump on every PR to main |

## Rationale
- **Manageable**: comment ratio + cppcheck keep code readable and clean
- **Secure**: semgrep + gitleaks catch vulnerabilities and leaked secrets before they hit GitHub
- **Flexible**: all checks run both locally (`make test`, `make check`) and in CI — same results everywhere
- **Discipline**: pre-commit hooks and branch protection enforce the workflow, not willpower

## Consequences
- Contributors must install `cloc`, `cppcheck` locally (or rely on CI feedback)
- Comment ratio threshold may need adjustment as codebase grows
- Every PR requires a VERSION bump — no exceptions
