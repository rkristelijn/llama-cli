# ADR-018: Feature Module Layout

## Status
Proposed

## Date
2026-04-11

## Context
The codebase has grown to 8 modules with 8 headers, 8 source files, and 13 test files spread across three flat directories (`src/`, `include/`, `test/`). When working on a feature, you need to jump between three directories. This doesn't scale and makes it hard to see what belongs together.

Goals:
- When working on a feature, all related files are in one place
- Small, focused files — no scrolling through 300+ line files
- Tests are co-located with source but excluded from the production binary
- Interfaces (pure virtual / type definitions) have their own file
- Familiar to developers coming from React/Angular (component = folder with all related files)
- Build system handles test exclusion automatically

## Decision

### Module structure
Each feature is a directory under `src/` containing all related files:

```
src/
  config/
    config.h              # public interface (types + declarations)
    config.cpp            # implementation
    config_test.cpp       # unit test
  json/
    json.h
    json.cpp
    json_test.cpp
  ollama/
    ollama.h              # public interface
    ollama.cpp
    ollama_types.h        # Message struct, ChatFn typedef (shared types)
  repl/
    repl.h
    repl.cpp
    repl_test.cpp
    repl_it.cpp           # integration test
  command/
    command.h
    command.cpp
    command_test.cpp
    commands_it.cpp
  annotation/
    annotation.h
    annotation.cpp
    annotation_test.cpp
    annotations_it.cpp
  exec/
    exec.h
    exec.cpp
    exec_test.cpp
  tui/
    tui.h                 # header-only (colors, markdown, spinner)
    markdown_it.cpp
  options/
    options_it.cpp        # integration test for /set, toggles
  main.cpp                # entry point, wires modules together
  test_helpers.h          # shared test utilities (MockLLM, test_cfg)
```

### Shared types
Types used across modules get their own header:
- `ollama_types.h` — `Message`, `ChatFn` (used by repl, ollama, tests)

This avoids circular dependencies and keeps interfaces small.

### Conventions
| Pattern | Meaning |
|---------|---------|
| `feature.h` | Public interface: types, declarations |
| `feature.cpp` | Implementation |
| `feature_types.h` | Shared types/interfaces used by other modules |
| `feature_test.cpp` | Unit test (one module in isolation) |
| `feature_it.cpp` | Integration test (feature flow end-to-end) |

### Build system
CMake uses glob or explicit lists per module:
- Production binary: `src/*/cpp` excluding `*_test.cpp` and `*_it.cpp`
- Test binaries: each `*_test.cpp` and `*_it.cpp` is a separate target
- Include path: `src/` so modules include as `#include "config/config.h"`

### Config and tooling
Tool configs live in `.config/`, scripts in `scripts/`:
```
.config/
  .clang-format           # code formatting rules
  .clang-tidy             # static analysis rules
  Doxyfile                # documentation generation
  pre-commit              # git hook: block main, auto-format

scripts/
  setup.sh                # install dev dependencies
  build-index.sh          # regenerate INDEX.md
  test_comment_ratio.sh   # enforce ≥20% comment ratio
  test_coverage.sh        # enforce ≥80% coverage per file
  pipeline-status.sh      # check CI pipeline status
  pr-status.sh            # show failed PR jobs
```

Tools reference configs via explicit paths (no symlinks):
- `clang-format --style=file:.config/.clang-format`
- `clang-tidy --config-file=.config/.clang-tidy`
- `doxygen .config/Doxyfile`

## Alternatives considered

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| Flat dirs (current) | Simple, familiar | Files scattered, doesn't scale | Rejected |
| `src/` + `include/` + `test/` (traditional C++) | Convention | 3 dirs per feature, no co-location | Rejected |
| Feature folders under `src/` | Co-located, scales, React/Angular-like | Needs CMake adjustment | **Chosen** |
| Monorepo with packages | Maximum isolation | Overkill for single binary | Rejected |

## Migration strategy
1. Move files one module at a time, starting with the simplest (json)
2. Update CMakeLists.txt include paths after each move
3. Run `make quick` after each module to catch breakage immediately
4. Update INDEX.md and imports last

## Consequences
- `#include "config.h"` becomes `#include "config/config.h"` (or just `"config.h"` with include path)
- All files for a feature are visible in one directory listing
- New features follow the pattern: create `src/feature/`, add .h, .cpp, _test.cpp
- Tests are co-located but excluded from production build by naming convention

## References
- @see ADR-008 (test framework — `_test` / `_it` naming)
- @see ADR-017 (integration tests)
- Inspired by Angular module pattern and React component folders
