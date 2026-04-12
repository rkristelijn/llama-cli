# ADR-024: Startup Precheck and Self-Remediation

*Status*: Accepted · *Date*: 2026-04-12 · *Context*: On a fresh Linux install, `make` failed because `cmake` was missing — with no actionable error. After a successful install, `llama-cli` started but connected to a non-existent model with no guidance on how to configure it.

## Problem

Two distinct failure modes with poor UX:

1. **Build-time**: `make` fails with a raw cmake error when cmake is not installed. The fix (`make setup`) is not mentioned.
2. **Runtime**: The tool starts, connects to Ollama, but the default model (`gemma4:e4b`) may not be pulled. The user gets a connection error with no hint on how to use a different model.

## Options Considered

| Option | Pros | Cons |
|--------|------|------|-
| Document in README only | Zero code | Easy to miss |
| Fail fast with actionable message | Clear, low effort | Requires guard in Makefile/main |
| **Precheck at build + runtime** | Best UX, self-remediating | Small code addition |

## Decision

### 1. Makefile guard for missing cmake

Add a `check-deps` target that runs before `all`, printing an actionable message if `cmake` is missing:

```makefile
check-deps:
	@command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Run 'make setup' first."; exit 1; }
```

### 2. Runtime precheck in `--help` and on startup

When `--help` is passed (or on first run), show:
- Current effective config (host, port, model)
- How to override via CLI or env var
- A connectivity check: can we reach Ollama?
- Which models are available (`ollama list`)

Example output:
```
llama-cli v1.x — local AI assistant

Config (use --help for all options):
  host:  localhost        (override: --host=X or OLLAMA_HOST=X)
  port:  11434            (override: --port=X or OLLAMA_PORT=X)
  model: gemma4:e4b       (override: --model=X or OLLAMA_MODEL=X)

Checking Ollama... OK
Available models: gemma4:e4b, llama3.2, mistral

Type 'exit' to quit.
```

If Ollama is unreachable:
```
Checking Ollama... FAILED
  Cannot connect to localhost:11434
  Is Ollama running? Try: ollama serve
  Use a different host: llama-cli --host=192.168.1.10
```

## Rationale

- Zani's Linux case: `make` with no cmake → now prints "Run 'make setup' first"
- Wrong model case: startup shows available models → user knows to pass `--model=llama3.2`
- Zero new dependencies — uses existing `Config` and HTTP client
- Consistent with ADR-004 (configuration) and ADR-011 (self-contained setup)

## Consequences

- `Makefile`: add `check-deps` prerequisite to `all` — gives actionable error before cmake fails cryptically
- `scripts/setup.sh`: replace `make install` with direct shell commands so setup works without cmake pre-installed
- `src/main.cpp`: add precheck function that prints config + probes Ollama
- ADR-011 onboarding section updated to mention `make setup` as first step when `make` fails
