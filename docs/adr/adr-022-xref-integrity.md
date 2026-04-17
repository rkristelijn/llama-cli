# ADR-022: Cross-Reference Integrity Checks

## Status

Proposed

## Context

This codebase is actively modified with AI assistance, which means bulk changes (moving files, renaming, restructuring) happen frequently. These changes can silently break internal references:

| Reference type | Example | Breaks when |
|---|---|---|
| Markdown links | `[see](docs/adr/adr-018.md)` | file renamed/moved |
| C++ `@see` tags | `@see docs/adr/adr-004.md` | doc renamed/moved |
| `#include` headers | `#include "config/config.h"` | header moved |
| Makefile script calls | `sh scripts/build-index.sh` | script renamed/moved |

Currently nothing catches these. A broken `@see` or dead markdown link goes unnoticed until someone reads the doc or the CI fails for an unrelated reason.

## Decision

Add a `make xref-check` target that validates all internal cross-references, and include it in `make check`.

### What to check

**1. Markdown links to local files** (in `docs/`, `README.md`, `CONTRIBUTING.md`):

```sh
grep -roh '(\./[^)]*\|[^)]*\.md)' docs/ README.md CONTRIBUTING.md \
  | grep -v 'http' \
  | while read -r ref; do [ -f "$ref" ] || echo "DEAD: $ref"; done
```

**2. C++ `@see` references to docs**:

```sh
grep -rh '@see ' src/ \
  | sed 's/.*@see //' \
  | while read -r ref; do [ -f "$ref" ] || echo "DEAD: $ref"; done
```

**3. Makefile script references**:

```sh
grep -oh 'scripts/[^ ]*\.sh' Makefile \
  | while read -r ref; do [ -f "$ref" ] || echo "DEAD: $ref"; done
```

**4. `#include` project headers** (relative, non-system):

```sh
grep -rh '#include "' src/ \
  | sed 's/.*#include "\(.*\)".*/\1/' \
  | while read -r ref; do [ -f "src/$ref" ] || echo "DEAD: src/$ref"; done
```

### Implementation

Single script `scripts/xref-check.sh`, exits non-zero if any dead reference is found. Added to `make check` and `make prepush`.

## Effort

Low. ~40 lines of shell. The patterns are already known from manual grep above. No new dependencies.

## Verdict

**Do it.** Reasons:

- **High value for AI-assisted workflows**: AI bulk-changes are the main risk vector. This catches the exact class of error that AI introduces most often (moving a file without updating all references).
- **Low effort**: pure shell, no new tools, fits the existing `make check` pattern.
- **Idempotent**: safe to run repeatedly, zero side effects.
- **Not overkill**: the repo already has comment ratio checks, coverage checks, format checks. Cross-reference integrity is the same category of structural quality gate.

The only thing it does *not* catch is references inside generated files (INDEX.md) or external URLs — those are out of scope.
