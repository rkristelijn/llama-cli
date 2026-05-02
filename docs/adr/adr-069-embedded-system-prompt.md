# ADR-069: Embedded System Prompt from Text File

*Status*: Implementing

## Date

2026-05-02

## Context

The system prompt is a large multi-line string defined inline in `src/config/config.h` using C++ string concatenation. This has several problems:

- Hard to read and edit (escaped quotes, no line breaks, no syntax highlighting)
- Difficult to review in PRs (changes to prompt logic are buried in C++ code)
- Encourages short, compressed phrasing instead of clear instructions
- Adding new capabilities (e.g. diagram rendering) requires touching C++ headers

The prompt needs to be easily editable as plain text while remaining a compile-time constant (no runtime file I/O dependency).

## Decision

1. Move the system prompt to `res/system-prompt.txt` — a plain text file
2. Use CMake `file(READ ...)` + `configure_file()` to embed the text as a C++ string literal at build time
3. Generate `src/config/system_prompt_generated.h` containing a `constexpr` string
4. The generated header is gitignored; the source of truth is the `.txt` file
5. Runtime override via `OLLAMA_SYSTEM_PROMPT` env var or `--system-prompt` flag still works

### Build pipeline

```text
res/system-prompt.txt
        │
        ▼  (cmake configure_file)
build/generated/system_prompt_generated.h
        │
        ▼  (include)
src/config/config.h  →  default value
```

### File format

`res/system-prompt.txt` is plain text. Newlines become spaces in the embedded string (single paragraph). Blank lines become paragraph breaks (`\n\n`).

## Consequences

- Prompt is readable and editable without C++ knowledge
- PRs that change prompt behavior are clearly visible in diff
- Build step is trivial (CMake only, no external tools)
- Generated file is deterministic — same input always produces same output
