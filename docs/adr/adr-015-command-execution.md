# ADR-015: Command Execution

*Status*: Accepted · *Date*: 2026-04-11 · *Context*: Users need to run shell commands from within the REPL — both directly and via LLM suggestions. This completes the roadmap item "Run commands and use output as context".

## Decision

Command execution is supported in three ways:

| Syntax | Who initiates | Output goes to | Confirmation |
|--------|--------------|----------------|-------------|
| `!command` | User | Terminal only | No (direct) |
| `!!command` | User | Terminal + LLM context | No (direct) |
| `<exec>command</exec>` | LLM | Terminal + LLM context | Yes (y/n) |

### `!command` — direct execution

Run a command, output to terminal. Not added to conversation history.

```text
> !ls src/
annotation.cpp  command.cpp  config.cpp  json.cpp  main.cpp  ollama.cpp  repl.cpp
```

### `!!command` — execute and feed to LLM

Run a command, show output, and add it to conversation history as context for the next LLM interaction.

```text
> !!make test 2>&1 | tail -5
[doctest] Status: SUCCESS!
PASS
> explain the test results
```

The output is added as a user message: `[command: make test 2>&1 | tail -5]\n<output>`.

### `<exec>command</exec>` — LLM-proposed execution

The LLM proposes a command in its response (per ADR-014 annotation pattern). The user confirms before execution. Output is fed back to the LLM for follow-up.

```text
User: "check if the tests pass"
LLM: "Let me run the tests: <exec>make test</exec>"
REPL: "Run: make test? [y/n]"
User: y
REPL: [runs, shows output, adds to history]
LLM: "All 5 test suites pass. The coverage is..."
```

### Parsing rules

| Input starts with | Type | Action |
|-------------------|------|--------|
| `!!` | ExecContext | Run command (after `!!`), add output to history |
| `!` | Exec | Run command (after `!`), show output only |
| Everything else | Existing logic | Prompt, slash command, or exit |

### Security

- Commands run as the current user with the current shell
- No sandboxing — same trust model as typing in a terminal
- LLM-proposed commands (`<exec>`) always require confirmation
- User-initiated commands (`!`, `!!`) execute immediately (user typed it)
- Output is truncated at 10,000 chars when added to LLM context (prevent token overflow)

### File reading

`!!cat <file>` replaces the earlier `/read` concept — it reads a file and adds the content to LLM context. The `!!` pattern is more general and works with any command.

### Configuration

New settings in Config (following ADR-004 precedence: CLI > env > defaults):

| Setting | CLI arg | Env var | Default | Purpose |
|---------|---------|---------|---------|---------|
| Exec timeout | `--exec-timeout` | `LLAMA_EXEC_TIMEOUT` | `30` | Max seconds for command execution |
| Max output | `--max-output` | `LLAMA_MAX_OUTPUT` | `10000` | Max chars of command output added to LLM context |

### Timeout and I/O handling

- Commands run via `popen()` with a read loop
- A timeout kills the child process after `exec_timeout` seconds
- Output is streamed to terminal in real-time (line by line)
- When added to LLM context, output is truncated at `max_output` chars with `[truncated]` marker
- stderr is merged with stdout (`2>&1` is implicit for `!!` and `<exec>`)

### Implementation

1. Extend `parse_input()` to detect `!` and `!!` prefixes
2. Add `cmd_exec()` — runs command via `popen()`, captures stdout+stderr
3. Extend `handle_response()` to parse `<exec>` annotations (same pattern as `<write>`)
4. Add output to history as `{"role": "user", "content": "[command: ...]\n<output>"}` for `!!` and `<exec>`

## Alternatives considered

| Alternative | Why not |
|-------------|---------|
| `/run command` | More typing, inconsistent with vi/IPython convention |
| Backtick syntax | Conflicts with markdown code blocks in LLM output |
| Auto-execute LLM commands | Unsafe — always require confirmation for LLM-initiated |
| Full vi mode (`:!`) | Colon conflicts with potential future prompt syntax |

## Consequences

- `popen()` introduces a system call — must handle errors and timeouts
- Large command output could flood LLM context — truncation needed
- `!!cat <file>` is the standard way to load files into LLM context
- System prompt must be updated to teach the LLM about `<exec>`
