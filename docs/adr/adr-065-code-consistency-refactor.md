# ADR-065: Code Consistency Refactor Plan

*Status*: Accepted

## Context

A code quality audit (2026-05-01) identified consistency issues and tech debt.
Quick wins (#2 duplicate escape_json, #3 duplicate parse_exec, #4 exit() in config,

## 10 pragma once) were fixed immediately. This ADR tracks the remaining items

### Remaining Items

#### High Priority — Split repl.cpp (2027 lines)

Split into focused modules:

- `repl_core.cpp` — loop + dispatch
- `repl_commands.cpp` — slash commands (/set, /color, /model, /scan, /mem, /pref, /rate)
- `repl_annotations.cpp` — write/read/exec/str_replace handling
- `repl_model.cpp` — model selection UI (handle_model_selection, 180 lines)

#### Medium Priority — Architecture

1. **Config singleton dual-access** — `ollama.cpp` takes `const Config& cfg` but also reads `Config::instance().trace`. Pick one pattern: either pass Config everywhere or use the singleton everywhere.
2. **tui.h is 821 lines inline** — move `StreamRenderer`, `Spinner`, and rendering functions to `tui.cpp`. Keep only declarations and trivial one-liners in the header.
3. **Inconsistent error output** — `main.cpp`/`config.cpp` use `std::cerr <<`, `repl.cpp`/`ollama.cpp` use `tui::error()`. Standardize on `tui::error()` for user-facing errors.
4. **process_sync_annotations (110 lines)** — extract each annotation type handler into its own function.

#### Low Priority — Code Hygiene

5. **File-slurp pattern repeated 14×** — extract `read_file(path) -> string` utility to `util.h`.
6. **Clipboard popen pattern repeated 5×** — extract `clipboard_copy()`/`clipboard_paste()` to `util.h`.
7. **Magic numbers** — replace hardcoded `120`, `10000`, `30`, `65535`, `11434` with named constants.
8. **cpp-linenoise pinned to `master`** — pin to a specific commit SHA.
9. **Dead code: `ollama_generate()`** — only used in tests, not production. Remove or document.
10. **Duplicate `read_file` / `read_file_or_empty`** in repl.cpp — consolidate.
11. **`.env` read-modify-write** duplicated between `save_to_dotenv()` and `handle_scan()`.

#### 12-Factor

12. **Non-reproducible builds** — `__DATE__`/`__TIME__` in main.cpp. Use git describe or build timestamp from CI instead.

### Decision

Items are tracked here for future sprints. Each can be done independently.
Priority order: split repl.cpp first (highest risk), then architecture items,
then hygiene.
