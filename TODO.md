# TODO

Status: `[ ]` open · `[-]` wip · `[>]` review · `[~]` blocked · `[x]` done · `[/]` cancelled — see [docs/task-status.md](docs/task-status.md)

Feature ideas and larger tasks live in [docs/backlog/](docs/backlog/README.md). This file tracks bugs and small fixes only.

## Bugs

- [x] `!!` command: AI doesn't always respond to the follow-up question
- [ ] config.cpp: `std::stoi(c.port)` accepts `11434abc` — verify entire port string is numeric
- [ ] config.cpp: `--files "/tmp/my notes.txt"` splits on whitespace — breaks quoted paths
- [ ] config.cpp: malformed numeric env/CLI values crash before `validate_config()` runs (unguarded `stoi`)
- [ ] config.cpp: TRACE handling inconsistent between .env (requires "true"/"1") and env var (any value)
- [ ] json_extract_object: brace counting doesn't handle braces inside quoted strings
- [ ] log-viewer.sh: JSON regex patterns break on spaced JSON (allow `[[:space:]]*` around colons)
- [ ] INDEX.md: duplicate ADR-003 numbering (0003-kiro-cli-as-reference.md vs adr-003-v-model-workflow.md)
- [ ] `/version` in REPL is malformed — possibly depending on dynamic value

## Small fixes

- [ ] Remove const_cast in main.cpp by changing load_config signature
- [ ] Reduce duplicate strings — e.g. connection message in main.cpp appears twice
- [ ] Replace magic literals with named constants (exit codes, tag strings, thresholds)
- [ ] Scan for duplicate code across source files and consolidate
- [ ] Refactor `src/logging/logger.cpp:log()` — exceeds function-size threshold
- [ ] Split `src/config/config.cpp` — more modularization needed
- [ ] `--provider` reused for tgpt upstream — model as separate `TGPT_PROVIDER` / `--tgpt-provider`
- [ ] ADR-027: exec capture claim inconsistent — add LOG_EVENT to exec flow or update ADR
- [ ] scripts/test/test-files-integration.sh: hardcoded paths — default to repo-relative
- [ ] scripts/test/test-files-integration.sh: tests don't assert expected content — benchmark, not test
- [ ] Optimize `make check` output — too chatty, find golden ratio of feedback
- [ ] Add logging tests (Logger::log, path(), JSONL format validation)
- [ ] Evaluate remaining 12-factor principles (#10)
- [x] The Makefile is a mess again, need to generate a tree view with what is calling what and verify the functionality is clean, maybe we should think about subcommands or grouping things
- [-] add !p and !c commands to copy the result or paste or should it be /copy /paste to keep it in line, should have a user/developer interaction design to keep things consistent and logic/intuitive (partial: `c` option added to all confirmation prompts)
- [ ] version should be bumped in pipeline not in merge request and it should bump properly feat/fix/breaking
- [ ] we should have /private to avoid logging
- [ ] Logger should detect dev vs installed context: log to `./events.jsonl` (repo-local) when running from source, `~/.llama-cli/events.jsonl` when installed — keeps dev and production logs separate
- [ ] Log viewer: configurable columns and column order (env var or config file) — e.g. `LLAMA_LOG_COLUMNS="time,agent,action,duration"`
- [ ] Add a dead code checker: detect unused scripts, unreferenced Makefile targets, orphaned files in `scripts/`
- [ ] Add a duplication checker: detect duplicate logic across scripts and source files (e.g. cpd, jscpd, or custom)
- [ ] up doesn't include ! and !!, when in private mode, /private commands, prompts are not added to the history
- [ ] commit and push hooks should keep absolute time, not only the indicudual tasks
- [ ] issue numbers online and local should be the same. issues should have proper body, not only a reference
- [ ] CI speed: add ccache to CI jobs (biggest win — avoids full rebuild on every push)
- [ ] CI speed: run coverage only on main merge, not on every PR push
- [ ] CI speed: combine sanitizers + unit-test jobs (sanitizers already runs all tests)
