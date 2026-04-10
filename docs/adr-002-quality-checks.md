# ADR-002: Quality Checks & CI Pipeline

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: This is a public repo meant to demonstrate structure and discipline. The codebase must stay manageable, secure, and flexible as it grows.

## Decision
The following automated quality gates are enforced on every PR:

| Tool | Purpose | Why |
|------|---------|-----|
| **cppcheck** | Static C++ analysis | Catches bugs, style issues, and undefined behavior before runtime |
| **semgrep** | SAST security scanning | Detects insecure patterns and common vulnerabilities |
| **gitleaks** | Secret detection | Prevents accidental credential leaks in a public repo |
| **cloc (20% comment ratio)** | Documentation enforcement | Ensures code is self-documenting and readable |
| **pre-commit hook** | Branch protection | Blocks direct commits to main, enforces feature branch workflow |
| **VERSION file** | Semver tracking | Version bump is verified by CI on every PR to main |

## Rationale
- **Manageable**: comment ratio + cppcheck keep code readable and clean
- **Secure**: semgrep + gitleaks catch vulnerabilities and leaked secrets before they reach GitHub
- **Flexible**: all checks run both locally (`make test`, `make check`) and in CI — same results everywhere
- **Discipline**: the workflow is enforced by pre-commit hooks and branch protection, not willpower

## Consequences
- `cloc` and `cppcheck` must be installed locally by contributors (or CI feedback can be relied upon)
- The comment ratio threshold may need adjustment as the codebase grows
- A VERSION bump is required for every PR — no exceptions
