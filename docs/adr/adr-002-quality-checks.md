# ADR-002: Quality Checks & CI Pipeline

*Status*: Accepted · *Date*: 2026-04-10 · *Updated*: 2026-04-11 · *Context*: This is a public repo meant to demonstrate structure and discipline. The codebase must stay manageable, secure, and flexible as it grows.

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
```

## Consequences
- Contributors need: `clang-tidy`, `clang-format`, `cppcheck`, `pmccabe`, `doxygen`, `cloc`, `gitleaks`, `semgrep`, `ccache`
- Sanitizers are opt-in locally (`-DENABLE_SANITIZERS=ON`) but always-on in CI
- The comment ratio threshold (20%) may need adjustment as the codebase grows

### Comment ratio strategy

The 20% minimum is enforced by `cloc` via `.config/test_comment_ratio.sh`. To stay structurally above the threshold (not chasing it line by line), every source file must have a `@file` doxygen header:

```cpp
/**
 * @file config.cpp
 * @brief Configuration loading: defaults → env vars → CLI args.
 * @see docs/adr/adr-004-configuration.md
 */
```

This serves three purposes:
1. Keeps comment ratio comfortably above 20% without padding
2. Links every file to its design rationale (ADR)
3. Makes `doxygen` output navigable with cross-references
- A VERSION bump is required for every PR — no exceptions
