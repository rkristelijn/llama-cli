# ADR-007: CLI Interface Design

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: The CLI interface is the user-facing API. It must follow POSIX conventions and support unix pipes for maximum composability.

## Decision

The CLI follows POSIX conventions and unix pipe semantics:

### Interface

```text
llama-cli [options] [prompt]
```

- **Options** start with `--` and configure behavior
- **Prompt** is a positional argument (the instruction)
- **Stdin** provides context data (file contents, pipe output)
- **Stdout** receives the response (pipeable)
- **Stderr** receives errors and status messages
- **`--`** marks end of options

### Options

| Long | Short | Env var | Default |
|------|-------|---------|---------|
| `--host` | `-h` | `OLLAMA_HOST` | `localhost` |
| `--port` | `-p` | `OLLAMA_PORT` | `11434` |
| `--model` | `-m` | `OLLAMA_MODEL` | `gemma4:e4b` |
| `--timeout` | `-t` | `OLLAMA_TIMEOUT` | `120` |

### Mode detection

| Stdin | Prompt arg | Mode |
|-------|-----------|------|
| TTY | none | Interactive (REPL) |
| TTY | present | Sync (one-shot) |
| pipe | none | Sync (stdin as prompt) |
| pipe | present | Sync (stdin as context, arg as instruction) |

### Examples

```bash
# Interactive — chat session
llama-cli

# Sync — one-shot with argument
llama-cli "explain the theory of relativity"

# Sync — pipe as prompt
echo "what is 2+2?" | llama-cli

# Sync — pipe as context, arg as instruction
cat main.cpp | llama-cli "review this code"

# Composable — chain with other tools
llama-cli "generate a .gitignore for C++" > .gitignore
cat error.log | llama-cli "explain this error" | tee analysis.md

# Options before prompt
llama-cli --model=gemma4:26b "explain quantum computing"
```

### Output rules

- Response text → stdout (clean, no decoration)
- Status messages ("Connecting to...") → stderr
- Errors → stderr
- This ensures pipes only receive the actual response

## Rationale

- POSIX conventions are followed so the tool behaves like any other unix tool
- Stdin/stdout separation enables composability with `|`, `>`, `<`, `tee`, `xargs`
- The `cat file | tool "instruction"` pattern matches how `ollama run`, `jq`, and `awk` work
- Status on stderr keeps stdout clean for piping

## Consequences

- `isatty(STDIN_FILENO)` is used to detect pipe vs interactive
- Status/progress output must go to stderr, not stdout
- Interactive mode needs a prompt indicator on stderr
