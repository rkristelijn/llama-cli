# TODO

## Refactoring
- [x] Extract JSON parsing from main.cpp into json.h/cpp (#7)
- [x] Extract Ollama client from main.cpp into ollama.h/cpp (#8)
- [x] Main should only wire config, client, and I/O

## Features
- [ ] Streaming responses (phase 2)
- [ ] Interactive prompt input
- [ ] File upload as context (phase 3)
- [ ] Run command and receive output (phase 4)
- [ ] Multi-host support: distribute load across machines (e.g. Raspberry Pi → Mac Studio)

## Quality
- [ ] Adopt test framework (Catch2/GoogleTest/doctest) for BDD-style tests (ADR needed)
- [ ] Add linter (clang-tidy) for code style enforcement
- [ ] Add complexity monitoring (pmccabe — lightweight local SonarQube alternative)
- [ ] Add config validation (port range, timeout > 0, non-empty host/model)
- [ ] Add integration test for ollama.cpp (requires mock or running Ollama)
- [ ] Add E2E test for main
- [ ] Evaluate remaining 12-factor principles (logging, port binding, etc.)
- [ ] Define release process (ADR)
