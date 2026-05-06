# ADR-002: Quality Checks & CI Pipeline

*Status*: Implemented · *Date*: 2026-04-10 · *Updated*: 2026-05-06 · *Context*: This is a public repo meant to demonstrate structure and discipline. The codebase must stay manageable, secure, and flexible as it grows.

## Decision

The following automated quality gates are enforced on every PR:

| Tool | Purpose | Why |
|------|---------|-----|
| **clang-tidy** | Style, naming, cognitive complexity, bugprone patterns | Catches subtle bugs and enforces consistent naming conventions |
| **pmccabe** | Cyclomatic complexity ≤ 10 per function | Hard limit on function complexity — keeps code easy to reason about |
| **cppcheck** | Static C++ analysis | Catches bugs, style issues, and undefined behavior before runtime |
| **doxygen** | Undocumented public API detection | Ensures every public function in headers is documented |
| **semgrep** | SAST security scanning | Detects insecure patterns and common vulnerabilities |
| **gitleaks** | Secret detection | Prevents accidental credential leaks in a public repo |
| **clang-format** | Consistent code formatting (Google style) | Auto-formats in pre-commit, verified in CI |
| **cloc (≥ 20% comment ratio)** | Documentation enforcement | Ensures code is self-documenting and readable |
| **ASan + UBSan** | Runtime memory/UB error detection | Catches bugs no static analyzer can find (leaks, overflows, UB) |
| **-Werror** | Warnings as errors | Prevents compiler warnings from accumulating |
| **ccache** | Build cache | Speeds up rebuilds by caching compilation results |
| **coverage (Codecov)** | Test coverage ≥ 80% per file | Ensures tests cover the codebase, enforced locally and in CI |
| **pre-commit hook** | Branch protection + auto-format | Blocks direct commits to main, formats staged files |
| **VERSION file** | Semver tracking | Version bump is verified by CI on every PR to main |

### Alternatives considered

| Choice | Alternative | Decision | Reason |
|--------|-------------|----------|--------|
| clang-tidy | splint, sparse | **clang-tidy** | Richer checks, configurable via `.clang-tidy`, active development |
| pmccabe | lizard, radon | **pmccabe** | Simple, Unix-philosophy, no dependencies |
| cppcheck | PVS-Studio, coverity | **cppcheck** | Free, open-source, good C++ support |
| clang-format | uncrustify, astyle | **clang-format** | De-facto standard for C/C++, integrates with clang-tidy |
| cloc | sloccount, tokei | **cloc** | Widely available, CSV output for scripting |
| doxygen | standardese, hdoc | **doxygen** | De-facto standard for C/C++, widely supported by editors |
| ASan/UBSan | valgrind | **ASan/UBSan** | Faster (2x vs 20x slowdown), better error messages, compile-time integration |
| ccache | sccache | **ccache** | More mature, wider adoption, simpler setup |
| Codecov | Coveralls | **Codecov** | Better GitHub integration, free for open source |
| MIT license | Apache 2.0, GPL | **MIT** | Most permissive, most popular, least friction for contributors |

## Rationale

- **Manageable**: comment ratio + cppcheck + clang-tidy + pmccabe keep code readable, clean, and simple
- **Documented**: doxygen lint ensures every public API is documented
- **Secure**: semgrep + gitleaks catch vulnerabilities and leaked secrets before they reach GitHub
- **Correct**: sanitizers catch runtime memory errors and undefined behavior that static analysis misses
- **Fast**: ccache speeds up rebuilds; -Werror prevents warning debt from accumulating
- **Flexible**: all checks run both locally (`make test`, `make check`) and in CI — same results everywhere
- **Discipline**: the workflow is enforced by pre-commit hooks and branch protection, not willpower
- **Fast CI**: path-based filtering skips code checks on docs-only PRs (dorny/paths-filter)

## CI Pipeline

### Quality target coverage

Every `make` target with a `##` comment is either in CI, in a hook, or on the denylist in `scripts/lint/check-pipeline-coverage.sh`. Run `make pipeline-coverage` to verify.

```text
make targets
├── pre-commit hook (scripts/git/precommit-check.sh)
│   ├── format-code      (if *.cpp/*.h staged)
│   ├── format-yaml      (if *.yml staged)
│   ├── format-md        (if *.md staged)
│   ├── format-scripts   (if *.sh staged)
│   ├── sast-stegano     (if images staged)
│   ├── sast-iac         (always)
│   ├── sast-secret      (always)
│   ├── check-pii        (if .pii file exists)
│   └── slop             (if *.cpp/*.h staged)
│
├── pre-push hook (scripts/git/prepush-check.sh)
│   ├── build            (if src/ changed)
│   ├── test-unit        (if src/ changed)
│   ├── lint-makefile    (if scripts/ changed)
│   ├── lint-scripts     (if scripts/ changed)
│   ├── comment-ratio    (if src/ changed)
│   └── sast-security    (always)
│
├── CI — ci.yml (Tier 2, runs on PR + main)
│   ├── lint-format-code
│   ├── lint-yaml
│   ├── lint-md
│   ├── lint-makefile
│   ├── lint-scripts
│   ├── lint-cppcheck
│   ├── lint-versions
│   ├── tidy
│   ├── complexity
│   ├── comment-ratio
│   ├── docs (doxygen)
│   ├── file-size
│   ├── dead-code
│   ├── dead-docs
│   ├── duplication
│   ├── build
│   ├── test-unit
│   ├── e2e
│   ├── feature-coverage
│   ├── coverage (+ Codecov + SonarCloud)
│   ├── sanitizers (ASan/UBSan)
│   ├── sast-security (semgrep)
│   ├── sast-secret (gitleaks)
│   ├── sast-trufflehog
│   ├── sast-grype
│   └── sast-checkov
│
├── check-all (Tier 3, exhaustive — local only)
│   ├── everything in CI above (via lint + test + sast)
│   ├── check-casts (slow, compiles each file)
│   ├── check-conversions (slow, full rebuild)
│   ├── check-shadowing (slow, compiles each file)
│   ├── check-traceability (needs feature-registry)
│   ├── pipeline-coverage (meta-check)
│   ├── mutation (30+ min)
│   └── sbom
│
├── lint aggregator (fast, grep/parse-based)
│   ├── lint-code (format + cppcheck)
│   ├── lint-md, lint-yaml, lint-makefile, lint-scripts, lint-versions
│   ├── tidy, complexity, comment-ratio, docs, file-size
│   ├── consistency, check-theme, check-xref
│   ├── check-interactive-input, check-pii, slop
│   ├── check-unicode
│   └── check-portability
│
└── denylist (intentionally not in CI — with reason)
    ├── mutation          — too slow (30+ min)
    ├── fuzz             — requires LLVM fuzzer
    ├── live, bench, preflight — requires running Ollama
    ├── summarize*       — requires Ollama LLM
    ├── sonar*           — requires SONAR_TOKEN
    ├── sast-codeql      — extremely slow (30+ min)
    ├── sast-iac         — trivy not in CI runner
    ├── sast-osv         — osv-scanner not in CI runner
    ├── sast-stegano     — zsteg (Ruby) not in CI runner
    └── format-*         — runs in pre-commit, not CI gate
```

### Path-based filtering

CI uses `dorny/paths-filter` to detect what changed. Code jobs only run when `src/`, `CMakeLists.txt`, or `Makefile` are modified. Version-bump and gitleaks always run.

### Local equivalents

| Command | When | What | Speed |
|---------|------|------|-------|
| `make quick` | During dev | incremental build + tests + comment ratio | ~5s |
| `make prepush` | Before push | smart: full check if code changed, index-only if docs | ~5s–60s |
| `make check` | Full audit | all 15 quality gates | ~60s |

### Monitoring CI from terminal

```bash
gh run list --limit 5                    # recent runs
gh run view <id>                         # run details
gh run view <id> --log-failed            # failure logs
gh run watch                             # live follow
gh pr checks                             # checks for current PR
```text

## Consequences

- Contributors need: `clang-tidy`, `clang-format`, `cppcheck`, `pmccabe`, `doxygen`, `cloc`, `gitleaks`, `semgrep`, `ccache`
- Sanitizers are opt-in locally (`-DENABLE_SANITIZERS=ON`) but always-on in CI
- The comment ratio threshold (20%) may need adjustment as the codebase grows

### Comment ratio strategy

The 20% minimum is enforced by `cloc` via `scripts/check/comment-ratio.sh`. To stay structurally above the threshold (not chasing it line by line), every source file must have a `@file` doxygen header:

```cpp
/**
 * @file config.cpp
 * @brief Configuration loading: defaults → env vars → CLI args.
 * @see docs/adr/adr-004-configuration.md
 */
```text

This serves three purposes:

1. Keeps comment ratio comfortably above 20% without padding
2. Links every file to its design rationale (ADR)
3. Makes `doxygen` output navigable with cross-references

- A VERSION bump is required for every PR — no exceptions
