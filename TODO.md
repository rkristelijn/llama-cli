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

## Features
- [x] Interactive prompt + REPL with conversation memory (#3, partial)
- [x] Sync mode — one-shot from CLI (#3, partial)
- [x] File read as context — replaced by `!!cat <file>`
- [x] File write from response — `<write file="path">content</write>`
- [x] Run command and receive output — `!`, `!!`, `<exec>`
- [ ] LLM system prompt: instruct to analyze command output, not repeat it
- [ ] TUI design ADR: colors, bold, layout, prompt style, consistent look & feel
- [ ] Markdown rendering in terminal (headings, bold, code blocks, lists)
- [ ] Mermaid diagram rendering (integrate mermaid-tui)
- [ ] Loading spinner/indicator while waiting for LLM response
- [ ] Streaming responses (#4)
- [ ] Async mode — fire-and-forget (#3, remaining)
- [ ] Stdin pipe support (ADR-007)
- [ ] Multi-host load distribution (#12)

## Quality
- [x] Adopt test framework — doctest with GWT style (#9)
- [x] Add linter — clang-format (Google/K&R) with auto-fix
- [x] Add complexity monitoring — clang-tidy + pmccabe
- [x] Coverage enforcement ≥ 80% per file
- [x] Sanitizers (ASan + UBSan) in CI
- [ ] Add config validation (port range, timeout > 0, non-empty host/model)
- [ ] Add integration test for ollama.cpp (requires mock or running Ollama)
- [ ] Add E2E test for main
- [ ] Evaluate remaining 12-factor principles (#10)
- [ ] Define release process (#11)
