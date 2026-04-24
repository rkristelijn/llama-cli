# Feature Coverage Matrix

Which features are tested where. Unit = doctest, E2E = bash script, Live = real LLM.

## Core

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| Sync mode (one-shot) | — | test_sync_mode | test_live | Positional arg triggers sync |
| Interactive REPL | test_repl | test_repl_mode | test_live | Linenoise input loop |
| Conversation history | test_repl | — | test_live | In-memory message list |
| `--session` multi-turn | — | test_session | — | JSON file persistence (ADR-056) |
| `--files` context | — | test_files_flag | — | File content as LLM context (ADR-030) |
| Stdin pipe | — | test_sync_mode | — | Pipe content as prompt |
| `--provider=mock` | — | all e2e | — | Deterministic echo for testing |
| Config: defaults | test_config | — | — | Struct defaults |
| Config: env vars | test_config | — | — | OLLAMA_HOST, etc. |
| Config: CLI args | test_config | — | — | --host, --port, etc. |
| Config: .env file | test_config | — | — | load_dotenv() |
| Config: precedence | test_config | — | — | CLI > env > .env > defaults |
| `--default-env` | test_config | — | — | Template generation |

## LLM Tool Annotations

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| `<write>` parse | test_annotation | — | — | Extract path + content |
| `<write>` confirm/skip | test_repl | test_smart_rw | test_live | y/n/t/c prompt |
| `<write>` auto-diff | test_repl | test_smart_rw | — | Myers LCS diff for existing files |
| `<str_replace>` parse | test_annotation | — | — | Extract path + old + new |
| `<str_replace>` apply | test_repl | test_smart_rw | test_live | Targeted replacement |
| `<str_replace>` diff | test_repl | test_smart_rw | — | Hunk headers + context |
| `<read>` parse | test_annotation | — | — | Extract path + lines + search |
| `<read>` inject | test_repl | test_smart_rw | test_live | File content → history |
| `<exec>` confirm/skip | test_repl | — | test_live | y/n/t/c prompt |
| Trust mode | test_repl | — | — | Auto-approve rest of session |
| Follow-up loop | test_repl | — | test_live | Re-send after read/exec output |
| Follow-up bounded | test_repl | — | — | Max iterations safety limit |
| Strip annotations | test_annotation | — | — | Clean display text |

## Capabilities (ADR-056)

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| `--capabilities=read` | — | test_session | — | Auto-execute `<read>` tags |
| Read-only exec allowlist | — | test_session | — | cat, ls, grep, etc. |
| Dangerous exec blocked | — | test_session | — | rm, curl, etc. blocked |
| `--capabilities=write` | — | test_session | — | Auto-execute `<write>`, `<str_replace>` |
| `--capabilities=exec` | — | — | — | ⚠️ Not yet tested |
| No capabilities = passthrough | — | test_session | — | Raw annotations in output |

## Sandbox (ADR-056)

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| `--sandbox=PATH` | — | test_session | — | Default: `.` |
| Read outside sandbox blocked | — | test_session | — | /etc/hostname blocked |
| Write outside sandbox blocked | — | test_session | — | ../outside.txt blocked |
| Read inside sandbox allowed | — | test_session | — | Files within sandbox OK |
| Symlink escape | — | — | — | ⚠️ Not yet tested |

## Commands

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| `/help` | test_command | — | — | Show available commands |
| `/set` | test_repl | — | — | Toggle runtime options |
| `/version` | test_repl | — | — | Show version info |
| `/model` | test_repl | — | — | Interactive model selection |
| `/read` | test_command | — | — | Manual file read |
| `/clear` | test_repl | — | — | Clear conversation history |
| `!command` | test_repl | — | test_live | Execute, output to terminal |
| `!!command` | test_repl | — | test_live | Execute, output as LLM context |

## Infrastructure

| Feature | Unit | E2E | Live | Notes |
|---------|------|-----|------|-------|
| JSON extraction | test_json | — | — | String, object, int |
| Command execution | test_exec | — | — | Timeout, output capture |
| Markdown rendering | test_markdown | — | — | Tables, code, bold, links |
| Stream rendering | test_markdown | — | — | Token-by-token markdown |
| Event logging | test_logger | — | — | JSONL format |
| Trace capture | test_trace | — | — | HTTP call tracing |

## Legend

- ✅ Covered by automated test
- ⚠️ Not yet tested (gap)
- `test_live` requires running Ollama (`make live`)
