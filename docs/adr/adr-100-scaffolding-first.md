---
title: "ADR-100: Scaffolding-First — Right the First Time"
status: accepted
date: 2026-05-07
---

## ADR-100: Scaffolding-First — Right the First Time

### Context

Analysis of 163 fix commits shows **~90 (55%) could have been prevented** if new files
were generated from templates that already satisfy all quality checks:

| Category | Fix commits | Root cause |
|----------|-------------|------------|
| Format/lint/doxygen | 36 | Files created without proper headers, comments, formatting |
| CI boilerplate | 24 | Jobs missing pinned SHAs, wrong runner, no path filter |
| Missing includes/platform | 16 | .cpp files missing `<sstream>`, `<memory>` for Linux/GCC |
| Test structure | 7 | Tests missing doctest boilerplate, wrong naming |
| File structure | 7 | Files in wrong location, missing CMake wiring |

The pattern: **every new file starts wrong, then gets fixed in 1-3 follow-up commits.**
A scaffolder eliminates this entire class of rework.

### Decision

#### 1. Template-based file generation via `make new`

```bash
make new TYPE=cpp NAME=parser/tokenizer    # src/parser/tokenizer.{h,cpp} + test
make new TYPE=test NAME=parser/tokenizer   # src/parser/tokenizer_test.cpp
make new TYPE=script NAME=lint/check-foo   # scripts/lint/check-foo.sh
make new TYPE=ci NAME=lint-foo             # CI job snippet (copy-paste ready)
make new TYPE=adr NAME=scaffolding         # docs/adr/adr-NNN-scaffolding.md
make new TYPE=module NAME=parser           # Full module: CMake + h + cpp + test
```

#### 2. Templates live in `templates/`

```text
templates/
├── cpp.h.tmpl          # Header with guards, doxygen, includes
├── cpp.cpp.tmpl        # Implementation with header, includes
├── cpp_test.cpp.tmpl   # Test with doctest, SCENARIO naming
├── script.sh.tmpl      # Bash with safety flags, TRACE, main()
├── ci-job.yml.tmpl     # CI job with pinned actions, path filter
├── adr.md.tmpl         # ADR with frontmatter, sections
└── module.tmpl         # Composite: generates h + cpp + test + CMake entry
```

#### 3. What each template guarantees

| Template | Guarantees (passes `make check` immediately) |
|----------|----------------------------------------------|
| `cpp.h` | Include guard, doxygen `@file`/`@brief`, common includes (`<string>`, `<vector>`), 20%+ comment ratio |
| `cpp.cpp` | Matching include, doxygen header, `<algorithm>` + `<sstream>` (Linux-safe) |
| `cpp_test` | `#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`, `SCENARIO("module: behavior")`, mock setup |
| `script.sh` | `#!/usr/bin/env bash`, `set -o errexit/nounset/pipefail`, TRACE block, `main "$@"` |
| `ci-job.yml` | Pinned checkout SHA, `ubuntu-24.04`, `needs: changes`, path filter |
| `adr.md` | YAML frontmatter (title, status, date), Context/Decision/Consequences sections |
| `module` | All of the above + CMakeLists.txt entry wired |

#### 4. Scaffolder validates output

After generation, the scaffolder runs:

```bash
clang-format --dry-run -Werror <generated .cpp/.h>
shellcheck <generated .sh>
```

If the template itself doesn't pass checks, the template is broken — fix the template, not the output.

#### 5. AI agents use scaffolders

The system prompt instructs AI agents:
> "When creating a new file, use `make new TYPE=... NAME=...` instead of writing from scratch."

This ensures AI-generated code also starts right.

### Consequences

- **~55% fewer fix commits** for new file creation
- Templates are the single source of truth for "what does a correct file look like"
- Adding a new lint rule → update templates → all future files comply automatically
- AI agents produce compliant code without needing to know all conventions
- Templates themselves are tested by CI (generate + check)

### Implementation Priority

1. **Phase 1**: `script.sh.tmpl` + `cpp.h.tmpl` + `cpp.cpp.tmpl` (covers 52 fix commits)
2. **Phase 2**: `cpp_test.cpp.tmpl` + `adr.md.tmpl` (covers 14 fix commits)
3. **Phase 3**: `ci-job.yml.tmpl` + `module.tmpl` (covers 24 fix commits)

### Alternatives Considered

- **Copier/cookiecutter**: Too heavy for single-file scaffolding. Good for full projects, overkill here.
- **IDE snippets**: Not portable, not CI-testable, not usable by AI agents.
- **Just document conventions**: Already tried — 163 fix commits prove documentation alone doesn't work.

### References

- [ADR-099: Right the First Time](adr-099-right-first-time.md) — tooling ROI and health metrics
- [ADR-018: Feature Module Layout](adr-018-module-layout.md) — where files go
- [ADR-048: Quality Framework](adr-048-quality-framework.md) — what checks must pass
