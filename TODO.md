# TODO

## Refactoring
- [ ] Extract JSON parsing from main.cpp into json.h/cpp
- [ ] Extract Ollama client from main.cpp into ollama.h/cpp
- [ ] Main should only wire config, client, and I/O

## Features
- [ ] Streaming responses (phase 2)
- [ ] Interactive prompt input
- [ ] File upload as context (phase 3)
- [ ] Run command and receive output (phase 4)
- [ ] Multi-host support: distribute load across machines (e.g. Raspberry Pi → Mac Studio)

## Quality
- [ ] Adopt test framework (Catch2/GoogleTest/doctest) for BDD-style tests (ADR needed)
- [ ] Evaluate remaining 12-factor principles (logging, port binding, etc.)
- [ ] Define release process (ADR)
- [ ] Add integration tests
- [ ] Add E2E tests
