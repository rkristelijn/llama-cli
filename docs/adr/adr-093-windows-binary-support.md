# ADR-093: Windows Binary Support

*Status*: Proposed · *Date*: 2026-05-01 · *Context*: Users have requested Windows support for the CLI tool.

## Problem

The release pipeline currently only builds for Linux (x86_64, arm64) and macOS (arm64). Windows users cannot run the CLI natively.

## Analysis

### Dependencies

| Dependency | Windows Support |
|------------|-----------------|
| cpp-linenoise | ✅ Has Win32_ANSI support |
| cpp-httplib | ✅ Has `_WIN32` support |
| dtl | ✅ C++ template library |
| doctest | ✅ C++ test framework |

### Required Code Changes

The codebase uses Unix-specific headers that need conditional compilation:

1. **`src/repl/repl.cpp`** — uses `<unistd.h>`, `<dirent.h>`, `<sys/stat.h>`
2. **`src/main.cpp`** — uses `<unistd.h>`

These need `#ifdef _WIN32` guards with Windows equivalents:

- `<windows.h>` for file operations
- `<dirent.h>` via pdcurses or compat library

### Release Pipeline

Add Windows to the build matrix in `.github/workflows/release.yml`:

```yaml
- os: windows-latest
  runner: windows-latest
  target: x86_64-pc-windows-msvc
  binary: llama-cli-windows-x64.exe
```text

## Decision

**Implement Windows support** with the following scope:

1. Add platform-specific includes in source files
2. Add Windows to release workflow matrix
3. Test on Windows runner

## Consequences

- Windows users can run the CLI natively
- Build matrix grows from 3 to 4 entries
- CI runtime increases by ~5 minutes
- Terminal colors require Windows 10+ for ANSI support
