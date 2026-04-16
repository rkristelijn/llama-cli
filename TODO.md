# TODO

## Refactoring
- [x] Extract JSON parsing from main.cpp into json.h/cpp (#7)
- [x] Extract Ollama client from main.cpp into ollama.h/cpp (#8)
- [x] Main should only wire config, client, and I/O
- [ ] Remove const_cast in main.cpp by changing load_config signature
- [ ] Rename project — "llama-cli" is confusing (LLaMA/llama.cpp/Ollama), needs model-agnostic name
- [ ] Reduce duplicate strings — e.g. connection message in main.cpp appears twice
- [ ] Replace magic literals with named constants (exit codes, tag strings, thresholds)
- [ ] Scan for duplicate code across source files and consolidate
- [x] Move non-core config files (Doxyfile, .clang-format, .clang-tidy) to `.config/`
- [x] Feature module layout: group .h, .cpp, test per feature in subdirectories
- [ ] Feature module decomposition: split monolithic files into focused modules

## Pre-release blockers
- [ ] Show diff preview before overwriting existing files (#50)
- [ ] /model command: list available models, switch model at runtime (#48)
- [ ] Improve system prompt: LLM repeats command output instead of analyzing it (#27)

## Features
- [x] Interactive prompt + REPL with conversation memory (#3, partial)
- [x] Sync mode — one-shot from CLI (#3, partial)
- [x] File read as context — replaced by `!!cat <file>`
- [x] File write from response — `<write file="path">content</write>`
- [x] Run command and receive output — `!`, `!!`, `<exec>`
- [x] LLM system prompt: instruct to analyze command output, not repeat it
- [x] TUI design ADR: colors, bold, layout, prompt style, consistent look & feel
- [x] Markdown rendering in terminal (headings, bold, code blocks, lists)
- [ ] Mermaid diagram rendering (integrate mermaid-tui)
- [x] Loading spinner/indicator while waiting for LLM response
- [x] Stdin pipe support (ADR-007)
- [x] --files flag support (ADR-030)
- [ ] CRUD permission settings: auto-allow read ops (cat, ls), confirm write/rename, block delete — configurable per operation type
- [ ] /model command: list available models, switch model at runtime (#48)
- [ ] /nick command: custom prompt label for multi-session use (#49)
- [ ] Clipboard integration: paste from clipboard as context, copy LLM response to clipboard
- [ ] Multi-host load distribution (#12)

## Documentation (junior dev review)
- [ ] Getting-started guide with golden path flow diagram (#44)
- [ ] Document state management within a REPL session (#45)
- [ ] Centralize configuration priority ladder (#46)
- [ ] Clarify LLM interaction model — suggesting vs executing (#47)

## Quality
- [ ] Pin all tool versions in `.config/versions.json` (doxygen [x], clang-format, clang-tidy, cppcheck, pmccabe, semgrep, gitleaks)
- [x] Adopt test framework — doctest with GWT style (#9)
- [x] Add linter — clang-format (Google/K&R) with auto-fix
- [x] Add complexity monitoring — clang-tidy + pmccabe
- [x] Coverage enforcement ≥ 80% per file
- [x] Sanitizers (ASan + UBSan) in CI
- [x] Add `make prepush` target (doxygen + format + clang-tidy, no semgrep/gitleaks/coverage)
- [x] Add config validation (port range, timeout > 0, non-empty host/model)
- [x] Upgrade doctest v2.4.12 → v2.5.1 (fixes CMake deprecation warning)
- [x] Integration test: mock LLM server, test all features end-to-end
- [ ] Add logging tests (Logger::log, path(), JSONL format validation)
- [ ] Evaluate remaining 12-factor principles (#10)
- [ ] Define release process (#11)
- [x] Fix chatty semgrep output in Makefile (use --quiet or filter marketing text)
- [x] Fix pmccabe "too many }'s" in `src/json/json_test.cpp` (lines 29, 44)
- [ ] Reduce complexity of `show_diff` and `confirm_write` in `src/repl/repl.cpp` (currently > 10)
- [x] Align local `make check` pmccabe threshold (15) with CI (10) and include test files
- [ ] Refactor `src/logging/logger.cpp:log()` — exceeds function-size threshold (ADR-027)
- [ ] Split `src/config/config.cpp` — parse_files_flag extracted, but more modularization needed
- [ ] cmake_minimum_required(VERSION 3.10) conflicts with the later FetchContent usage. The FetchContent module was introduced in CMake 3.11, so builds targeting CMake 3.10 will fail during configuration.
- [ ] exec capture claim is inconsistent with current implementation. Line 77 states exec events are already captured, but src/exec/exec.cpp:28-65 currently executes commands and returns ExecResult without emitting LOG_EVENT(...). Either update this ADR statement or add the missing logging in exec flow.
- [ ] --provider is already the app-level provider selector. Config::provider already selects backends like ollama and mock. Reusing the same setting for tgpt's internal upstream choice makes the CLI ambiguous. This should be modeled as a separate tgpt-specific option such as TGPT_PROVIDER / --tgpt-provider.
- [ ] scripts/test-files-integration.sh path hardcoded, this makes the script fail out of the box anywhere except that workstation, including CI. Default these repo-relative locations instead.
- [ ] scripts/test-files-integration.sh For Tests 1, 2, 3, and 5, status stays ✅ even if the binary returns the wrong answer or an immediate error. That makes this script a benchmark harness, not a reliable integration test. Please assert expected content/exit behavior, similar to e2e/test_files_flag.sh.
- [ ] config.cpp std::stoi(c.port) accepts values like 11434abc and returns 11434, but the original invalid string is used directly to build the client URL at http:// + host + : + port, creating a malformed URL. Verify the entire port string is numeric.
- [ ] config.cpp Whitespace tokenization breaks quoted file paths. --files "/tmp/my notes.txt" reaches this code as one argv entry, but iss >> path splits it into two paths. That makes valid filenames containing spaces impossible to pass through --files.
- [ ] config.cpp Malformed numeric env/CLI values still abort before validate_config() runs. load_env() and match_int_opts() call std::stoi() without exception handling on OLLAMA_TIMEOUT, LLAMA_EXEC_TIMEOUT, LLAMA_MAX_OUTPUT (from env), and numeric CLI options. Inputs like OLLAMA_TIMEOUT=abc or --max-output=NaN throw unhandled exceptions before validate_config() is reached. Only the port validation in validate_config() includes a try-catch block; the other numeric fields assume already-parsed integers.

## Release & Distribution
- [ ] Prepare repo for first release (tag, changelog, version bump)
- [ ] Publish to Homebrew (brew tap or core formula)
- [ ] Publish to apt (PPA or .deb package)
- [ ] Write user documentation (install, usage, config, examples)
- [ ] make compile watch: https://stackoverflow.com/questions/931093/how-do-i-make-my-program-watch-for-file-modification-in-c
- [ ] when executing !! the ai doesn't respond to question
- [ ] discover whether it is possible to include autocompletion with tab when using paths, e.g. if it starts with ~, . or /
- [ ] optimize and cleanup the make check, when succes it doesn't say always, some tools are just too chatty, there must be a golden ratio, enough to have overview and not too much to overflow
- [ ] llama-cli --make-config makes example .env with all options and a short description what it does (self documenting)


