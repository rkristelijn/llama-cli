# Architecture Decision Records

Summary of all decisions made in this project.

| # | Decision | Choice | Date |
|---|----------|--------|------|
| [001](adr-001-http-library.md) | HTTP library | cpp-httplib (header-only, FetchContent) | 2026-04-10 |
| [002](adr-002-quality-checks.md) | Quality checks | cppcheck, semgrep, gitleaks, comment ratio | 2026-04-10 |
| [003](adr-003-v-model-workflow.md) | Development workflow | Define → Build → Test → Audit → Release | 2026-04-10 |
| [004](adr-004-configuration.md) | Configuration | Env vars + CLI args, 12-factor | 2026-04-10 |
| [005](adr-005-execution-modes.md) | Execution modes | Interactive, sync, async | 2026-04-10 |
| [006](adr-006-branching-strategy.md) | Branching strategy | GitHub Flow, tag at milestones | 2026-04-10 |
| [007](adr-007-cli-interface.md) | CLI interface | POSIX conventions, unix pipes | 2026-04-10 |
| [008](adr-008-test-framework.md) | Test framework | doctest, GWT style, function pointer mocking | 2026-04-10 |
| [009](adr-009-code-formatting.md) | Code formatting | clang-format (Google/K&R), auto-fix in pre-commit | 2026-04-10 |
| [010](adr-010-documentation-indexing.md) | Documentation indexing | Frontmatter + INDEX.md, dogfooded | 2026-04-10 |
