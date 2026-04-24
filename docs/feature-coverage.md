# Feature Coverage Matrix

Which features are tested where. Unit = doctest, E2E = bash script, Live = real LLM.

## Core

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| Sync mode (one-shot) | — | test_sync_mode | test_live | |
| Interactive REPL | test_repl | test_repl_mode | test_live | |
| Conversation history | test_repl | — | test_live | |
| `--session` multi-turn | test_config | test_session | — | |
| `--files` context | — | test_files_flag | — | ⚠️ No unit test for file loading |
| Stdin pipe | — | test_sync_mode | — | |
| `--provider=mock` | — | all e2e | — | |
| Config: defaults | test_config | — | — | |
| Config: env vars | test_config | — | — | |
| Config: CLI args | test_config | — | — | |
| Config: .env file | test_config | — | — | |
| Config: precedence | test_config | — | — | |
| `--default-env` | test_config | — | — | |
| Streaming responses | — | — | test_live | ⚠️ No mock test for streaming |
| `--no-color` | test_config | — | — | |
| `--no-banner` | — | — | — | ⚠️ Not tested |
| `--why-so-serious` | — | — | — | ⚠️ Not tested |
| `--repl` (force REPL) | — | — | — | ⚠️ Not tested |

## LLM Tool Annotations

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| `<write>` parse | test_annotation | — | — | |
| `<write>` confirm/skip | test_repl | test_smart_rw | test_live | |
| `<write>` auto-diff | test_repl | test_smart_rw | — | |
| `<str_replace>` parse | test_annotation | — | — | |
| `<str_replace>` apply | test_repl | test_smart_rw | test_live | |
| `<str_replace>` diff | test_repl | test_smart_rw | — | |
| `<read>` parse | test_annotation | — | — | |
| `<read>` inject | test_repl | test_smart_rw | test_live | |
| `<read>` line range | test_annotation | — | — | ⚠️ No e2e test |
| `<read>` search | test_annotation | — | — | ⚠️ No e2e test |
| `<exec>` confirm/skip | test_repl | — | test_live | |
| Trust mode | test_repl | — | — | |
| Follow-up loop | test_repl | — | test_live | |
| Follow-up bounded | test_repl | — | — | |
| Strip annotations | test_annotation | — | — | |
| Copy to clipboard | test_repl | — | — | |

## Capabilities (ADR-056)

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| `--capabilities=read` | test_config | test_session | — | |
| Read-only exec allowlist | — | test_session | — | ⚠️ No unit test for is_read_only() |
| Dangerous exec blocked | — | test_session | — | |
| `--capabilities=write` | test_config | test_session | — | |
| `--capabilities=exec` | — | — | — | ⚠️ Not tested |
| No capabilities = passthrough | — | test_session | — | |
| Redirect `>` blocked in read mode | — | — | — | ⚠️ Not tested |
| Pipe safety (each segment checked) | — | — | — | ⚠️ Not tested |

## Sandbox (ADR-056)

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| `--sandbox=PATH` | test_config | test_session | — | |
| Read outside sandbox blocked | — | test_session | — | |
| Write outside sandbox blocked | — | test_session | — | |
| Read inside sandbox allowed | — | test_session | — | |
| New file write (parent dir check) | — | — | — | ⚠️ Not tested |
| Symlink escape | — | — | — | ⚠️ Not tested |
| `..` traversal blocked | — | — | — | ⚠️ Not tested (realpath handles it) |

## Commands

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| `/help` | test_command | — | — | |
| `/set` | test_repl | — | — | |
| `/version` | test_repl | — | — | |
| `/model` | test_repl | — | — | |
| `/read` | test_command | — | — | |
| `/clear` | test_repl | — | — | ⚠️ Only checks output, not history reset |
| `!command` | test_repl | — | test_live | |
| `!!command` | test_repl | — | test_live | ⚠️ No test that output enters history |

## Infrastructure

| Feature | Unit | E2E | Live | Gap |
|---------|------|-----|------|-----|
| JSON string extraction | test_json | — | — | |
| JSON object extraction | test_json | — | — | |
| JSON int extraction | test_json | — | — | |
| JSON unicode escape | test_json | — | — | |
| JSON missing key | test_json | — | — | |
| Command execution | test_exec | — | — | |
| Exec timeout | test_exec | — | — | |
| Exec output truncation | test_exec | — | — | |
| Exec failed command | test_exec | — | — | |
| Markdown rendering | test_markdown | — | — | |
| Table rendering | test_markdown | — | — | |
| Stream rendering | test_markdown | — | — | |
| Event logging (JSONL) | test_logger | — | — | |
| Logger path detection | test_logger | — | — | |
| Logger escape chars | test_logger | — | — | |
| Trace capture | test_trace | — | — | |
| StderrTrace smoke | test_trace | — | — | |
| Ollama API: generate | — | — | test_live | ⚠️ Needs HTTP mock |
| Ollama API: chat | — | — | test_live | ⚠️ Needs HTTP mock |
| Ollama API: streaming | — | — | test_live | ⚠️ Needs HTTP mock |
| Ollama API: model list | — | — | — | ⚠️ Needs HTTP mock |
| Ollama API: retry on failure | — | — | — | ⚠️ Needs HTTP mock |

## Coverage Summary

| Module | Lines | Coverage | Status |
|--------|-------|----------|--------|
| annotation.cpp | 146 | ~92% | ✅ Good |
| command.cpp | 31 | 100% | ✅ Complete |
| config.cpp | 276 | ~73% | 🔶 OK — new flags tested |
| exec.cpp | 36 | ~86% | ✅ Good |
| json.cpp | 106 | ~81% | 🔶 OK |
| logger.cpp | 61 | ~75% | 🔶 OK |
| ollama.cpp | 205 | 0% | 🔴 Needs HTTP mock |
| repl.cpp | 744 | ~54% | 🔶 Large file, many paths |
| trace.cpp | 16 | ~37% | 🔶 StderrTrace hard to capture |
| tui.h | 466 | ~76% | 🔶 OK via markdown_it |

## Low-Hanging Fruits

Tests that would improve coverage with minimal effort:

| What | Where | Effort | Impact |
|------|-------|--------|--------|
| `is_read_only()` unit test | main.cpp or extract to module | Low | Validates allowlist logic |
| `--capabilities=exec` e2e | test_session.sh | Low | Covers yolo mode |
| `..` traversal sandbox test | test_session.sh | Low | Validates realpath |
| Redirect `>` blocked test | test_session.sh | Low | Validates pipe safety |
| `!!` injects into history | test_repl.cpp | Low | Verifies context injection |
| `/clear` resets history | test_repl.cpp | Low | Verifies state reset |
| `--no-banner` suppresses banner | test_repl.cpp | Low | Trivial flag check |
| Config: malformed port | test_config.cpp | Low | Bug from TODO.md |
| Logger: dev vs installed path | test_logger.cpp | Medium | Needs env manipulation |
| Ollama API mock | new test file | High | Needs HTTP mock server |

## Codecov Badge Setup

The coverage badge in README uses [Codecov](https://codecov.io). Setup:

1. Log in at <https://codecov.io> with your GitHub account (free for public repos)
2. Navigate to the repo settings and copy the "Repository Upload Token"
3. In GitHub: Settings → Environments → create environment named `workflow`
4. Add secret `CODECOV_TOKEN` with the token value
5. The CI `test-coverage` job references `environment: workflow` to access the secret
6. After merge to main, the badge updates automatically
