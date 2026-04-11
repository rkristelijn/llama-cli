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
- [ ] Streaming responses (#4)
- [ ] Async mode — fire-and-forget (#3, remaining)
- [ ] Stdin pipe support (ADR-007)
- [ ] CRUD permission settings: auto-allow read ops (cat, ls), confirm write/rename, block delete — configurable per operation type
- [ ] /model command: list available models, switch model at runtime (#48)
- [ ] /nick command: custom prompt label for multi-session use (#49)
- [ ] Multi-host load distribution (#12)

## Documentation (junior dev review)
- [ ] Getting-started guide with golden path flow diagram (#44)
- [ ] Document state management within a REPL session (#45)
- [ ] Centralize configuration priority ladder (#46)
- [ ] Clarify LLM interaction model — suggesting vs executing (#47)

## Quality
- [x] Adopt test framework — doctest with GWT style (#9)
- [x] Add linter — clang-format (Google/K&R) with auto-fix
- [x] Add complexity monitoring — clang-tidy + pmccabe
- [x] Coverage enforcement ≥ 80% per file
- [x] Sanitizers (ASan + UBSan) in CI
- [x] Add `make prepush` target (doxygen + format + clang-tidy, no semgrep/gitleaks/coverage)
- [ ] Add config validation (port range, timeout > 0, non-empty host/model)
- [ ] Upgrade doctest v2.4.12 → v2.5.1 (fixes CMake deprecation warning)
- [x] Integration test: mock LLM server, test all features end-to-end
- [ ] Add E2E test for main
- [ ] Evaluate remaining 12-factor principles (#10)
- [ ] Define release process (#11)

## Release & Distribution
- [ ] Prepare repo for first release (tag, changelog, version bump)
- [ ] Publish to Homebrew (brew tap or core formula)
- [ ] Publish to apt (PPA or .deb package)
- [ ] Write user documentation (install, usage, config, examples)
