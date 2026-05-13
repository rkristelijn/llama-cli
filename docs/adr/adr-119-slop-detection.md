---
summary: 25 deterministic grep-based patterns in check-slop.sh detect AI-generated code anti-patterns (pass-through wrappers, catch-log-rethrow, AI naming, uniform line length, emoji bullets, em-dash overuse) in git diffs, backed by arxiv research and slop-scan benchmarks.
status: accepted
---

# ADR-119: AI Slop Detection Strategy

## Context

AI-assisted development produces code that compiles and passes tests but accumulates technical debt through predictable anti-patterns. GitClear's 2025 study of 211M lines found duplicated code grew 4-8x, refactoring dropped 60%, and AI-heavy code generates 9x more churn. We need automated detection that catches these patterns before merge.

## Decision

Implement deterministic, grep-based slop detection in `scripts/lint/check-slop.sh` scanning git diff against main. Patterns are organized into C++ code patterns and markdown documentation patterns, each backed by research.

## Pattern Taxonomy

### C++ Code Patterns (1-17)

| # | Pattern | Source | Signal |
|---|---------|--------|--------|
| 1 | Redundant comments restating code | slop-guard, remove-ai-slop | Comments explain WHAT not WHY |
| 2 | Empty catch blocks | slop-scan | Errors silently swallowed |
| 3 | Paranoid nullptr + "should never" | remove-ai-slop | Defensive checks on validated paths |
| 4 | Excessive TODO/FIXME | slop-guard | Unfinished AI output shipped |
| 5 | Blank line bloat (>15%) | GitClear churn data | Padding without purpose |
| 6 | Repeated boilerplate (4+ dupes) | GitClear duplication study | Copy-paste over abstraction |
| 7 | Trivial docstrings | slop-guard | Restating the function name |
| 8 | Over-commenting (>40%) | Nirob et al. | AI narrates everything |
| 9 | Abstraction inflation | slop-guard, Larridin | Interfaces without 2+ impls |
| 10 | Cargo cult std::move | Community | Pointless on literals |
| 11 | Semantic duplication | GitClear, slop-scan | Same logic repeated |
| 12 | AI tell-words in comments | antislop-sampler | delve/leverage/seamless/orchestrate |
| 13 | Pass-through wrappers | slop-scan (top rule) | Functions that just forward |
| 14 | Catch-log-rethrow | slop-guard | Adds nothing, hides intent |
| 15 | AI naming (Helper/Manager/Utils) | r/dotnet, slop-guard | Generic role suffixes |
| 16 | Debug output left behind | remove-ai-slop | std::cout in non-debug code |
| 17 | Uniform line length (stddev<8) | Nirob et al. 2026 | AI monotony signal |

### Markdown Documentation Patterns (18-25)

| # | Pattern | Source | Signal |
|---|---------|--------|--------|
| 18 | AI vocabulary | antislop-sampler | delve/holistic/paradigm/ecosystem |
| 19 | Filler phrases | Artificial Ignorance | "as technology continues to evolve" |
| 20 | Over-hedging | Artificial Ignorance | "it's important to note" |
| 21 | Em-dash overuse (>10) | Artificial Ignorance, Reddit data | Usage tripled since LLM adoption |
| 22 | Servile positivity | Community | game-changer/cutting-edge/revolutionize |
| 23 | Snappy triads | Artificial Ignorance | Three-beat rhetorical cliches |
| 24 | Unicode formatting | Artificial Ignorance | Bold/italic via Unicode math symbols |
| 25 | Emoji bullet points | Artificial Ignorance, remove-ai-slop | Professional docs with emoji lists |

## Research Sources

### Academic Papers

1. **Nirob et al. 2026** "Whitespaces Don't Lie" (arxiv 2601.19264) — Feature-based detection achieves 0.995 AUC. Top discriminators: avg leading spaces, avg leading tabs, blank-line ratio, AST depth. Whitespace patterns are the strongest signal.
2. **Baltes et al. 2026** "AI Slop and the Software Commons" (arxiv 2604.16754) — Frames AI slop as tragedy of the commons. Documents: setTimeout masking race conditions, casting to `any`, rewriting tests to accept broken behavior.
3. **GitClear 2025** AI Copilot Code Quality Report (211M lines) — Duplicated blocks grew 4-8x, moved lines (refactoring) dropped 60%, short-term churn 9x higher for heavy AI users.

### Tools and Repositories

1. **slop-scan** (github.com/modem-dev/slop-scan, 208 stars) — Deterministic JS/TS scanner. Benchmarked: AI repos score 6.91x higher than mature OSS on normalized metrics.
2. **slop-guard** (github.com/pj-costello/slop-guard) — Anti-slop rules + Python linter. Categories: over-abstraction, unnecessary comments, defensive over-engineering, kitchen-sink deps.
3. **/remove-ai-slop** (ramarivera gist) — Claude slash command. Patterns: explanatory comments, null checks on validated paths, verbose patterns where terse ones are idiomatic.
4. **antislop-sampler** (github.com/sam-paech/antislop-sampler) — Token-level suppression of overused LLM phrases.

### Community Sources

- **Artificial Ignorance** (Charlie Guo, Substack) — "The Field Guide to AI Slop": em-dash conspiracy data, snappy triads, Unicode formatting, parallelism overuse, monotonous structure.
- **r/dotnet** — "Normalize()" appearing excessively in AI code; high file count with 90% code in few files.
- **Larridin** — AI Slop Index: code duplication ratio, revert rates, complexity-adjusted analysis, architectural coherence, test behavior coverage.

## Design Principles

1. **Deterministic** — grep-based, no ML models, no network calls, reproducible results.
2. **Portable** — No `grep -P`, no GNU-only flags. Works on macOS, Linux, Alpine, CI.
3. **Low false-positive** — Thresholds set conservatively; single instances are fine, patterns in aggregate are flagged.
4. **Non-blocking** — Reports warnings, does not fail the build. Developers decide.
5. **Diff-based** — Only scans new code (git diff against main), not the entire codebase.

## Shell Portability (check-portability.sh)

Added 8 shell portability checks to prevent non-portable patterns in scripts:

| Pattern | Problem | Alternative |
|---------|---------|-------------|
| `grep -P` | GNU-only, fails on macOS/BSD/Alpine | `grep -E` or literal chars |
| `sed -i "..."` | GNU vs BSD syntax differs | `sed -i '' "..."` or temp file |
| `readarray`/`mapfile` | bash 4+ only | `while IFS= read -r` loop |
| `declare -A` | bash 4+ only | case/if statements |
| `seq` | Not POSIX | `{1..N}` brace expansion |
| `echo -e` | Behavior varies | `printf '%s\n'` |
| `stat -c` | GNU-only | `wc -c < file` for size |
| `date -d` | GNU-only | `date -j -f` (BSD) with fallback |

## Consequences

- Every PR is automatically scanned for 25 slop patterns.
- Developers get immediate feedback on AI-typical anti-patterns.
- Shell scripts are checked for cross-platform compatibility.
- False positives are possible but thresholds are tuned to minimize them.
- New patterns can be added as research evolves — each must cite a source.

## References

- @see scripts/lint/check-slop.sh
- @see scripts/lint/check-portability.sh
- @see docs/adr/adr-048-quality-framework.md
- @see docs/adr/adr-097-cpp-quality-checks.md
