# ADR-012: Interactive REPL

*Status*: Accepted · *Date*: 2026-04-10 · *Updated*: 2026-04-11 · *Context*: Interactive mode needs a read-eval-print loop. The REPL must be testable, support graceful exit, and be upgradeable to line editing later.

## Decision

The REPL uses `cpp-linenoise` for interactive input (arrow keys, history) and falls back to `std::getline` for non-interactive streams (tests).

### Input handling

| Input | Behavior |
|-------|----------|
| Text + Enter | Sent to Ollama, response printed |
| Empty line | Skipped |
| `exit` or `quit` | Clean exit |
| Ctrl+C | Interrupts LLM call, returns to prompt |
| EOF (Ctrl+D) | Clean exit |
| Arrow up/down | Navigate command history |

### Testability

The generate function and I/O streams are injected, allowing tests to use string streams and mock responses without HTTP calls (ADR-008).

### Line editing

`std::getline` is used for non-`std::cin` streams (tests, pipes). Interactive mode uses `linenoise::Readline` with arrow key history. The stream identity check (`&in != &std::cin`) ensures linenoise is never invoked during tests.

## Rationale

- `cpp-linenoise` is header-only, BSD licensed, same author as cpp-httplib
- Interactive mode uses `linenoise::Readline` with arrow key history
- Tests still use `std::getline` via injectable `std::istream` — no linenoise dependency in tests
- SIGINT handler installed during LLM calls: chat runs on a detachable thread, Ctrl+C returns to prompt immediately

## Consequences

- Arrow key navigation and command history work out of the box
- Ctrl+C gracefully interrupts LLM calls (abandoned HTTP thread finishes in background)
- Tests remain fast and deterministic (no terminal interaction)
- Stream identity check prevents linenoise from being invoked during unit tests
