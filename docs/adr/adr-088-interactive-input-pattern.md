---
summary: The file provides detailed guidance on using `linenoise::Readline()` for reliable confirmation prompts in C++ programs, due to issues with `std::cin` and `std::getline` handling after raw mode manipulation by linenoise.
status: accepted
---

summary: The file provides detailed guidance on using `linenoise::Readline()` for reliable confirmation prompts in C++ programs, due to issues with `std::cin` and `std::getline` handling after raw mode manipulation by linenoise.

# ADR-088: Interactive Input Pattern — Use linenoise for All User Prompts

## Context

The REPL uses [cpp-linenoise](https://github.com/yhirose/cpp-linenoise) for the main prompt. Linenoise manipulates the terminal (raw mode: disables ICANON, ICRNL, ISIG, ECHO). After `Readline()` returns, it restores the original terminal settings.

However, `std::cin` / `std::getline` becomes **unreliable** after linenoise has manipulated the terminal. This is a [known issue](https://github.com/yhirose/cpp-linenoise/issues/10) — the stream buffer enters an undefined state after raw mode manipulation. Symptoms:

- `^M` displayed instead of Enter being processed (CR not translated to LF)
- Ctrl-C does not work (ISIG was disabled, may not be fully restored for cin)
- Inconsistent behavior across platforms (Linux vs macOS)

This affected all y/n confirmation prompts (write, str_replace, exec).

## Decision

**All interactive user input MUST use `linenoise::Readline()`** — never `std::cin` or `std::getline(std::cin, ...)`.

### The Pattern

One reusable function handles both interactive and test modes:

```cpp
static bool read_answer(std::istream& in, std::string& answer) {
  if (&in == &std::cin && isatty(STDIN_FILENO)) {
    // Interactive: linenoise handles raw mode, Enter, Ctrl-C, echo
    auto quit = linenoise::Readline("", answer);
    return !quit;
  }
  // Non-interactive (tests, pipes): std::getline is safe
  if (!std::getline(in, answer)) return false;
  if (!answer.empty() && answer.back() == '\r') answer.pop_back();
  return true;
}
```

### Why This Works

- Linenoise manages raw/cooked mode switching internally per call
- Ctrl-C returns `quit=true` — caller handles gracefully
- Enter key processed correctly (linenoise handles CR→LF internally)
- Tests pass `std::istringstream` — falls through to `std::getline` (safe, no terminal)
- Same pattern already proven in `repl_model.cpp` (model selection)

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| `tcsetattr()` before each `std::getline` | Fragile, race conditions, doesn't fix cin buffer state |
| `read(STDIN_FILENO, ...)` directly | Reinvents readline (no echo, no editing, no history) |
| Strip `\r` from `std::getline` result | Workaround, doesn't fix Ctrl-C, unreliable |
| Switch to GNU readline | Heavy dependency, licensing (GPL), overkill |

## Enforcement

A lint script (`scripts/lint/check-interactive-input.sh`) detects violations:

- `std::getline(std::cin` — direct cin reads
- `std::cin >>` — direct cin extraction
- `std::cin.get(` — single char reads from cin

Exceptions: the `read_answer()` function itself (guarded by `isatty` check).

## Consequences

- All confirmation prompts now support Enter, Ctrl-C, and proper echo
- Tests remain fast (stringstream, no terminal interaction)
- New interactive prompts must use `read_answer()` — enforced by lint
- The `linenoise.hpp` include is needed in files that call `read_answer()`

## References

- [cpp-linenoise issue #10: Cannot work with stdin after linenoise](https://github.com/yhirose/cpp-linenoise/issues/10)
- [SO: How to get cin to read a raw mode terminal](https://stackoverflow.com/questions/63715108)
- [SO: cin stream buffer undefined after raw mode](https://stackoverflow.com/questions/63715108/how-to-get-cin-to-read-a-raw-mode-terminal#63732819)
