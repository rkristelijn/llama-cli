# User Guide

## Table of contents

1. [Prerequisites](#1-prerequisites)
2. [Installation](#2-installation)
3. [Quick start](#3-quick-start)
4. [Interactive mode](#4-interactive-mode)
5. [Running commands](#5-running-commands)
6. [File operations](#6-file-operations)
7. [Model selection](#7-model-selection)
8. [Sync mode](#8-sync-mode)
9. [REPL commands](#9-repl-commands)
10. [Configuration](#10-configuration)
11. [Event logging](#11-event-logging)
12. [Troubleshooting](#12-troubleshooting)

## 1. Prerequisites

- [Ollama](https://ollama.com) running locally or on a reachable host
- At least one model pulled: `ollama pull gemma4:e4b`

See [ollama-setup.md](ollama-setup.md) for detailed setup instructions.

## 2. Installation

### One-liner (recommended)

```bash
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash
```

Auto-detects OS and architecture (Linux x64/arm64, macOS arm64), downloads the release, verifies checksum, installs to `/usr/local/bin`.

```bash
# Custom install directory
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | INSTALL_DIR=~/.local/bin bash

# Specific version
curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | VERSION=0.18.1 bash
```

### From source

```bash
brew install ollama
brew services start ollama
ollama pull gemma4:e4b
make setup
sudo make install
```

## 3. Quick start

```bash
llama-cli                          # start interactive chat
> hello, what can you do?          # type a prompt
> !!cat src/main.cpp               # feed a file to the LLM
> what does this code do?          # ask about it
> exit                             # quit
```

## 4. Interactive mode

Start with `llama-cli` (no arguments). The LLM remembers the full conversation.

```text
> what is the capital of France?
Paris.
> and of Germany?
Berlin.
```

Exit with `exit`, `quit`, or Ctrl+D. Interrupt a running LLM call with Ctrl+C.

## 5. Running commands

### `!command` — run and display

Runs the command, prints output to terminal. Output is **not** sent to the LLM.

```text
> !ls src/
config/  exec/  logging/  repl/  main.cpp
```

### `!!command` — run and add to context

Runs the command, prints output, **and** adds it to the LLM conversation as context.

```text
> !!cat src/main.cpp
[output shown...]
> what does this code do?          # LLM can now see the file
```

### LLM-proposed commands

The LLM can propose commands using `<exec>`. You always confirm before execution:

```text
> run the tests
Run: make test? [y/n]
```

- `y` — execute the command
- `n` — skip

## 6. File operations

### File writes

The LLM can propose creating or overwriting files:

```text
> create a .gitignore for C++
[proposed: write .gitignore]
Write to .gitignore? [y/n/s/d]
```

- `y` — write the file (existing files backed up to `.bak`)
- `n` — skip
- `s` — show the proposed content first
- `d` — show diff against existing file (red = removed, green = added)

### Targeted edits (str_replace)

For existing files, the LLM prefers targeted edits over full rewrites:

```text
> fix the typo on line 42
[proposed: str_replace src/repl/repl.cpp]
- old text
+ new text
Apply str_replace to src/repl/repl.cpp? [y/n]
```

Existing file is backed up to `.bak` before applying.

### File reading

The LLM can read files to understand your code:

```text
> what does repl.cpp do?
[read src/repl/repl.cpp]
```

It supports line ranges and search:

- `<read path="file"/>` — full file
- `<read path="file" lines="10-20"/>` — line range
- `<read path="file" search="TODO"/>` — search with context

## 7. Model selection

Switch models at runtime without restarting:

```text
> /model
Available models:
  1. gemma4:e4b
  2. gemma4:26b
  3. llama3:8b
Select model (1-3): 2
[model set to gemma4:26b]
```

Or set the model at startup:

```bash
llama-cli --model=llama3:8b
OLLAMA_MODEL=mistral llama-cli
```

List available models: `ollama list`
Pull a new model: `ollama pull llama3.2`

## 8. Sync mode

One-shot mode for scripts and pipes. Response goes to stdout, status to stderr.

```bash
# One-shot question
llama-cli "explain what a Makefile does"

# Pipe output to a file
llama-cli "generate a .gitignore for C++" > .gitignore

# File as context
llama-cli --files src/main.cpp "review this code"

# Stdin as context
cat error.log | llama-cli "explain this error"
```

## 9. REPL commands

| Command | Description |
|---------|-------------|
| `!command` | Run command, output to terminal |
| `!!command` | Run command, output as LLM context |
| `/clear` | Clear conversation history |
| `/model` | Select a model from the server |
| `/color` | Set prompt or AI response color |
| `/set` | Show runtime options |
| `/set <opt>` | Toggle option (`markdown`, `color`, `bofh`, `trace`) |
| `/version` | Show version info |
| `/help` | Show available commands |
| `exit`, `quit` | Exit the REPL |
| Ctrl+C | Interrupt running LLM call |
| Ctrl+D | Exit (EOF) |

## 10. Configuration

Settings are loaded in this order (last wins):

```text
defaults → environment variables → .env file → CLI flags
```

### All settings

| Setting | CLI flag | Env var | Default |
|---------|----------|---------|---------|
| Provider | `--provider` | `LLAMA_PROVIDER` | `ollama` |
| Host | `--host` | `OLLAMA_HOST` | `localhost` |
| Port | `--port` | `OLLAMA_PORT` | `11434` |
| Model | `--model` | `OLLAMA_MODEL` | `gemma4:e4b` |
| Timeout | `--timeout` | `OLLAMA_TIMEOUT` | `120` |
| Exec timeout | `--exec-timeout` | `LLAMA_EXEC_TIMEOUT` | `30` |
| Max output | `--max-output` | `LLAMA_MAX_OUTPUT` | `10000` |
| No color | `--no-color` | `NO_COLOR` | auto-detect TTY |
| BOFH mode | `--why-so-serious` | — | `false` |
| System prompt | — | `OLLAMA_SYSTEM_PROMPT` | (built-in) |
| Files | `--files` | — | — |
| Trace | `--trace` | `TRACE` | `false` |

### .env file

Create a project-local config file:

```bash
llama-cli --default-env > .env
```

Edit to uncomment and change values. The `.env` file is git-ignored by default.

## 11. Event logging

All actions are logged to `~/.llama-cli/events.jsonl` as structured JSON (one event per line). This runs automatically — no configuration needed.

### What is logged

| Event | When | Fields |
|-------|------|--------|
| `session_start` | REPL starts | model, host:port |
| `session_end` | REPL exits | prompt count |
| `exec` | `!command` | command, output, duration_ms |
| `exec_context` | `!!command` | command, output, duration_ms |
| `exec_confirmed` | LLM command accepted | command, output, duration_ms |
| `exec_declined` | LLM command rejected | command |
| `file_write` | File written | path, ok/error |
| `file_write_declined` | File write rejected | path |
| `str_replace` | Edit applied | path, ok/error |
| `str_replace_declined` | Edit rejected | path |
| `file_read` | File read by LLM | path |
| `generate` / `chat` | LLM call | prompt, response, duration_ms, tokens |

### Viewing logs

```bash
make log                        # last 50 events as table
make log ARGS="exec"            # filter to exec events only
make log ARGS="chat -n 10"     # last 10 chat events
```

Example output:

```text
TIME       AGENT      ACTION               DURATION     IN    OUT  SUMMARY
---------- ---------- ------------------ -------- ------ ------  -------
06:27:14   repl       session_start           0ms      0      0  gemma4:26b
06:28:48   ollama     chat                15086ms    252    192  [{"role":"user"...
06:35:26   repl       exec_context           41ms      0      0  cat ~/.llama-cli/events.jsonl
```

### Example log entry

```json
{
  "timestamp": "2026-04-19T08:15:00.123Z",
  "agent": "repl",
  "action": "exec",
  "input": "make test",
  "output": "[doctest] Status: SUCCESS!",
  "duration_ms": 1200,
  "tokens_prompt": 0,
  "tokens_completion": 0
}
```

### Why logging matters

- **Debug** — see what happened when something goes wrong
- **Optimize** — find expensive prompts and slow commands
- **Audit** — track which files the LLM read and wrote

See [ADR-027](adr/adr-027-event-logging.md) for the design rationale.

## 12. Troubleshooting

### Connection refused

```text
Error: could not connect to localhost:11434
```

Ollama isn't running. Start it:

```bash
# macOS
brew services start ollama

# Linux
systemctl --user start ollama
```

### Model not found

```text
Error: model "llama3" not found
```

Pull the model first: `ollama pull llama3`

### Slow responses

- Try a smaller model: `llama-cli --model=gemma4:e4b` (9.6GB)
- Check if another process is using the GPU: `nvidia-smi` or `Activity Monitor`
- Increase timeout for large prompts: `llama-cli --timeout=300`

### Trace mode

Enable verbose output to see HTTP calls and internal state:

```bash
llama-cli --trace
# or in REPL:
/set trace
```
