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
| [011](adr-011-self-contained-setup.md) | Self-contained setup | make setup, portable macOS/Linux | 2026-04-10 |
| [012](adr-012-interactive-repl.md) | Interactive REPL | std::getline, injectable I/O, upgradeable | 2026-04-10 |
| [013](adr-013-file-reading.md) | File reading & commands | /read, stdin pipe, slash commands | 2026-04-10 |
| [014](adr-014-tool-annotations.md) | LLM tool annotations | XML tags in response, user confirmation | 2026-04-10 |
| [015](adr-015-command-execution.md) | Command execution | `!cmd`, `!!cmd` (context), `<exec>` (LLM) | 2026-04-11 |
| [016](adr-016-tui-design.md) | TUI design | ANSI 16-color, TTY detect, `tui::` namespace | 2026-04-11 |
| [017](adr-017-integration-tests.md) | Integration tests | Gherkin specs + doctest, mock ChatFn | 2026-04-11 |
