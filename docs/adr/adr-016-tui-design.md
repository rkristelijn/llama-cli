# ADR-016: TUI Design

*Status*: Accepted

## Date

2026-04-11

## Context

All output currently uses plain `std::cout` / `std::cerr` with no visual distinction between user input, LLM responses, system messages, errors, and command output. This makes it hard to scan conversations, especially with `!` / `!!` / `<exec>` output mixed in.

We need a consistent visual language that:

- Distinguishes message types at a glance
- Works in all terminals (256-color not guaranteed)
- Stays readable when piped (`cmd | less`, redirected to file)
- Doesn't require ncurses or external TUI libraries
- Is testable (injectable output stream)

## Decision

### Color scheme (ANSI 16-color, bold)

| Element | Style | ANSI code | Example |
|---------|-------|-----------|---------|
| User prompt | bold white | `\033[1m` | `> hello` |
| LLM response | default (no color) | — | plain text |
| System message | dim/gray | `\033[2m` | `Connected to...` |
| Error | bold red | `\033[1;31m` | `Error: timeout` |
| Command output (`!`) | cyan | `\033[36m` | `ls` output |
| Exec context (`!!`) | dim cyan | `\033[2;36m` | file contents |
| Write proposal | yellow | `\033[33m` | `[write src/main.cpp]` |
| Prompt marker | bold green | `\033[1;32m` | `>` |

### Implementation

- Single header `src/tui/tui.h` with inline functions
- Auto-detect TTY: color only when `isatty(STDOUT_FILENO)` is true
- `--no-color` CLI flag and `NO_COLOR` env var override (per <https://no-color.org>)
- Every styled print resets with `\033[0m` after output
- All output goes through `tui::` functions, never raw ANSI in source files

### API (minimal)

```cpp
namespace tui {
  void prompt(std::ostream& out);           // print "> " in green
  void system(std::ostream& out, const std::string& msg);  // dim gray
  void error(std::ostream& out, const std::string& msg);   // bold red
  void cmd_output(std::ostream& out, const std::string& msg); // cyan
  void write_proposal(std::ostream& out, const std::string& path); // yellow
  bool use_color();  // check TTY + NO_COLOR + --no-color
}
```

### Also implemented in this PR

- Markdown rendering: headings (bold+underline), **bold**, *italic*, `code` (cyan), ```code blocks``` (cyan), bullet/numbered lists — see [ADR-052](adr-052-markdown-renderer.md) for full renderer design
- Loading spinner (RAII, only on TTY) with BOFH mode (`--why-so-serious`)
- Arrow key history via cpp-linenoise
- `/set` command: toggle markdown, color, bofh at runtime
- `/version` command with git dirty detection
- Ctrl+C interrupt: SIGINT handler + detachable chat thread

### Future

- Mermaid diagram rendering (integrate mermaid-tui)

## Alternatives considered

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| ncurses | Full TUI, windows, panels | Heavy dependency, hard to test, breaks piping | Rejected |
| FTXUI | Modern C++ TUI | Large dependency, overkill for chat | Rejected |
| 256-color / truecolor | Prettier | Not supported in all terminals | Rejected |
| No color at all | Simple, always works | Hard to scan output | Rejected |
| ANSI 16-color | Universal, no deps, pipe-safe with TTY detect | Limited palette | **Chosen** |

## Consequences

- All `std::cerr <<` and `std::cout <<` calls in repl/main get replaced with `tui::` calls
- `Config` gets `no_color` bool field
- Tests can verify output without ANSI codes (non-TTY streams)
- Follows <https://no-color.org> convention

## References

- @see ADR-012 (REPL design — injectable I/O)
- @see ADR-015 (command execution — output types)
- @see <https://no-color.org>
