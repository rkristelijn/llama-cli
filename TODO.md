# TODO

## Pre-release blockers
- [ ] /model command: list available models, switch model at runtime (#48)
- [ ] Streaming responses (UX bottleneck — user waits for full response)
- [ ] Inline code rendering in markdown (backticks visible in output)

## Bugs
- [ ] `!!` command: AI doesn't always respond to the follow-up question
- [ ] config.cpp: `std::stoi(c.port)` accepts `11434abc` — verify entire port string is numeric
- [ ] config.cpp: `--files "/tmp/my notes.txt"` splits on whitespace — breaks quoted paths
- [ ] config.cpp: malformed numeric env/CLI values crash before `validate_config()` runs (unguarded `stoi`)
- [ ] config.cpp: TRACE handling inconsistent between .env (requires "true"/"1") and env var (any value)
- [ ] json_extract_object: brace counting doesn't handle braces inside quoted strings
- [ ] log-viewer.sh: JSON regex patterns break on spaced JSON (allow `[[:space:]]*` around colons)
- [ ] INDEX.md: duplicate ADR-003 numbering (0003-kiro-cli-as-reference.md vs adr-003-v-model-workflow.md)

## Features
- [ ] Mermaid diagram rendering (integrate mermaid-tui)
- [ ] CRUD permission settings: auto-allow read ops, confirm write/rename, block delete
- [ ] /nick command: custom prompt label for multi-session use (#49)
- [ ] Clipboard integration: paste from clipboard as context, copy response to clipboard
- [ ] Multi-host load distribution (#12)
- [ ] Tab autocompletion for file paths (when input starts with `~`, `.` or `/`)
- [ ] `llama-cli --make-config` generates example .env with all options documented
- [ ] Sync mode session ID for follow-up questions (conversational sync mode)
- [ ] `make compile watch` — auto-rebuild on file changes

## Refactoring
- [ ] Remove const_cast in main.cpp by changing load_config signature
- [ ] Rename project — "llama-cli" is confusing (LLaMA/llama.cpp/Ollama)
- [ ] Reduce duplicate strings — e.g. connection message in main.cpp appears twice
- [ ] Replace magic literals with named constants (exit codes, tag strings, thresholds)
- [ ] Scan for duplicate code across source files and consolidate
- [ ] Feature module decomposition: split monolithic files into focused modules
- [ ] Refactor `src/logging/logger.cpp:log()` — exceeds function-size threshold
- [ ] Split `src/config/config.cpp` — more modularization needed
- [ ] `--provider` reused for tgpt upstream — model as separate `TGPT_PROVIDER` / `--tgpt-provider`
- [ ] ADR-027: exec capture claim inconsistent — add LOG_EVENT to exec flow or update ADR

## Quality
- [ ] Pin all tool versions in `.config/versions.json` (cppcheck, pmccabe, semgrep, gitleaks)
- [ ] Add logging tests (Logger::log, path(), JSONL format validation)
- [ ] Evaluate remaining 12-factor principles (#10)
- [x] Define release process (#11) — GitHub Actions workflow, VERSION file, tag workflow
- [ ] scripts/test-files-integration.sh: hardcoded paths — default to repo-relative
- [ ] scripts/test-files-integration.sh: tests don't assert expected content — benchmark, not test
- [ ] Optimize `make check` output — too chatty, find golden ratio of feedback

## Documentation
- [ ] Getting-started guide with golden path flow diagram (#44)
- [ ] Document state management within a REPL session (#45)
- [ ] Centralize configuration priority ladder (#46)
- [ ] Clarify LLM interaction model — suggesting vs executing (#47)

## Release & Distribution
- [ ] Prepare repo for first release (tag, changelog, version bump)
- [ ] Publish to Homebrew (brew tap or core formula)
- [ ] Publish to apt (PPA or .deb package)
- [ ] Write user documentation (install, usage, config, examples)
ed
