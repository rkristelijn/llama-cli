# ADR-023: Self-Documenting Processes

## Status
Accepted

## Context

This codebase is actively maintained with AI assistance, which means:
- Bulk changes happen fast, mistakes happen silently
- Junior devs and AI both need clear feedback to course-correct
- A failing check with no explanation wastes a round-trip (costs credits, costs time)

The goal is that every automated check teaches, not just blocks.

## Decision

Every quality gate must be self-documenting: when it fails, it explains *why* the rule exists and *how* to fix it.

### Levels of self-documentation

**1. Error messages with rationale + tips**

Bad:
```
FAIL: comment ratio 19% is below the 20% threshold
```

Good:
```
FAIL: comment ratio 19% is below the 20% threshold

Why this matters: comments help junior devs and AI understand intent,
not just mechanics. Every file should have @file/@brief headers and
inline comments for non-trivial logic.

Tips:
  - Run 'make comment-ratio' to see which files are lowest
  - Add @brief/@param/@return to functions missing them
  - When asking AI to write code, include: 'keep comment ratio >= 20%'
```

Applied to: `scripts/test_comment_ratio.sh`

**2. Tests as documentation**

Unit tests (`*_test.cpp`) document the contract of a module — what inputs produce what outputs.
Integration tests (`*_it.cpp`) document scenarios — what a user does and what they see.

Every scenario in an integration test is a sentence:
```cpp
SCENARIO("user runs !! command, output is added to history") { ... }
SCENARIO("user types /set markdown, markdown is toggled off") { ... }
```

These are readable by anyone, including AI, without understanding the implementation.

**3. `@see` links from code to decisions**

Every non-trivial implementation choice links to its ADR:
```cpp
/**
 * @file repl.cpp
 * @brief Interactive REPL loop with conversation memory and commands.
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-014-tool-annotations.md
 */
```

This means: "if you wonder *why* this works this way, read the ADR."

**4. `make help` as the entry point**

Every script callable via `make <target>`. `make help` lists all targets with one-line descriptions. No tribal knowledge required.

**5. INDEX.md as navigation**

`INDEX.md` is auto-generated and always current. It gives AI (and humans) a map of the repo without reading every file.

## Automated check: self-documentation opportunities

### Is it worth it?

Yes, with scope limits. A full "is this comment meaningful?" check requires NLP and is overkill. But structural gaps are detectable with simple grep:

| Check | Pattern | Detectable? |
|---|---|---|
| Missing `@file` header | `.cpp`/`.h` without `@file` | ✅ easy |
| Missing `@brief` on public functions | `.h` function without `@brief` above | ✅ easy |
| Script without usage comment | `.sh` without `# ` in first 5 lines | ✅ easy |
| Make target without help entry | target in Makefile not in `make help` | ✅ medium |
| Error message without tips | `exit 1` without preceding `echo` tip | ⚠️ fragile |
| Test without SCENARIO description | `TEST_CASE` without meaningful name | ⚠️ subjective |

### Proposed script: `scripts/selfdoc-check.sh`

```sh
#!/bin/sh
# selfdoc-check.sh — Check for missing self-documentation opportunities
set -e
FAIL=0

# 1. C++ files missing @file header
for f in src/**/*.cpp src/**/*.h; do
  grep -q '@file' "$f" || { echo "MISSING @file: $f"; FAIL=1; }
done

# 2. Shell scripts missing a usage comment in first 5 lines
for f in scripts/*.sh; do
  head -5 "$f" | grep -q '^#' || { echo "MISSING header comment: $f"; FAIL=1; }
done

# 3. Make targets missing from help output
targets=$(grep -E '^[a-z-]+:' Makefile | cut -d: -f1 | grep -v '^\.')
for t in $targets; do
  grep -q "make $t" Makefile || { echo "MISSING help entry: make $t"; FAIL=1; }
done

[ "$FAIL" -eq 0 ] && echo "PASS" || exit 1
```

**Effort**: low — ~30 lines, no new dependencies.
**Value**: medium — catches the most common structural gaps without false positives.
**Verdict**: add to `make check`. Skip the fragile checks (exit 1 without tips, subjective test names).

### Future: LLM-assisted quality check

The structural checks above catch *missing* documentation. Checking whether existing comments are *meaningful* requires understanding context — that's where a local LLM fits naturally.

Approach: pipe each function without its comment to `ollama run gemma4:e4b` and ask "does this comment add value or just restate the code?" Zero external dependencies, runs offline, fits the existing privacy model of this repo.

Not implemented yet — noted here as a natural next step once the structural checks are in place.

## Consequences

- Every new script must have a header comment
- Every new C++ file must have `@file` + `@brief`
- Every new make target must appear in `make help`
- Error messages in scripts should include rationale and at least one tip
- AI instructions for this repo should include: "keep comment ratio >= 20%, add @see to ADRs"

## References

- @see docs/adr/adr-002-quality-checks.md — comment ratio enforcement
- @see docs/adr/adr-008-test-framework.md — doctest SCENARIO convention
- @see docs/adr/adr-010-documentation-indexing.md — INDEX.md generation
- @see docs/adr/adr-017-integration-tests.md — integration test scenarios
- @see docs/adr/adr-022-xref-integrity.md — cross-reference checks
- @see docs/credits-in-ai.md — AI-efficient workflow tips
