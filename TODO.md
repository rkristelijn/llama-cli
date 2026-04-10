# TODO

## Refactoring
- [x] Extract JSON parsing from main.cpp into json.h/cpp (#7)
- [x] Extract Ollama client from main.cpp into ollama.h/cpp (#8)
- [x] Main should only wire config, client, and I/O

## Features
- [x] Interactive prompt + REPL with conversation memory (#3, partial)
- [x] Sync mode — one-shot from CLI (#3, partial)
- [ ] Async mode — fire-and-forget (#3, remaining)
- [ ] Streaming responses (#4)
- [ ] Stdin pipe support (ADR-007)
- [ ] File read as context (#5)
- [ ] File write from response (#6)
- [ ] Run command and receive output
- [ ] Multi-host load distribution (#12)

## Quality
- [x] Adopt test framework — doctest with GWT style (#9)
- [x] Add linter — clang-format (Google/K&R) with auto-fix
- [ ] Add complexity monitoring (pmccabe)
- [ ] Add config validation (port range, timeout > 0, non-empty host/model)
- [ ] Add integration test for ollama.cpp (requires mock or running Ollama)
- [ ] Add E2E test for main
- [ ] Evaluate remaining 12-factor principles (#10)
- [ ] Define release process (#11)
