# ADR-012: Interactive REPL

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: Interactive mode needs a read-eval-print loop. The REPL must be testable, support graceful exit, and be upgradeable to line editing later.

## Decision
The REPL is implemented with `std::getline` and injectable I/O streams.

### Input handling
| Input | Behavior |
|-------|----------|
| Text + Enter | Sent to Ollama, response printed |
| Empty line | Skipped |
| `exit` or `quit` | Clean exit |
| Ctrl+C | Process killed by OS (standard unix behavior) |
| EOF (Ctrl+D) | Clean exit |

### Testability
The generate function and I/O streams are injected, allowing tests to use string streams and mock responses without HTTP calls (ADR-008).

### Line editing
`std::getline` is used initially — no arrow keys, no history. This can be upgraded to `linenoise` or `readline` later without changing the REPL logic, since input comes through `std::istream`.

## Rationale
- `std::getline` has zero dependencies and is sufficient for an MVP
- Injectable I/O makes the REPL fully unit-testable
- Ctrl+C as process kill is standard unix behavior — no signal handler needed until state (e.g. chat history) must be saved
- Line editing libraries can be added later as a drop-in upgrade

## Consequences
- No arrow key navigation or command history in the initial version
- No graceful Ctrl+C handling — acceptable until chat history is persisted
