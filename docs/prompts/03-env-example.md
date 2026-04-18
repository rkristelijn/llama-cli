# Prompt 03: Add .env.example configuration template

## Context

- The project uses environment variables for configuration (see README.md Configuration table)
- The CLI supports `--default-env > .env` to generate a config file
- There is no committed `.env.example` for new contributors
- `.env` is gitignored

## Task

Create `.env.example` at the project root with all documented settings.

## File: Create `.env.example`

```bash
# llama-cli configuration
# Copy this file: cp .env.example .env
# Then edit values as needed.
# @see docs/adr/adr-004-configuration.md

# Ollama connection
OLLAMA_HOST=localhost
OLLAMA_PORT=11434
OLLAMA_MODEL=gemma4:e4b
OLLAMA_TIMEOUT=120

# Execution limits
LLAMA_EXEC_TIMEOUT=30
LLAMA_MAX_OUTPUT=10000

# Display (uncomment to disable color)
# NO_COLOR=1

# Custom system prompt (uncomment to override built-in)
# OLLAMA_SYSTEM_PROMPT="You are a helpful assistant."
```

## Verify

```bash
# 1. File exists
test -f .env.example && echo "PASS: .env.example exists" || echo "FAIL: file not found"

# 2. Contains all required vars
grep -c "OLLAMA_HOST" .env.example && echo "PASS: has OLLAMA_HOST"
grep -c "OLLAMA_PORT" .env.example && echo "PASS: has OLLAMA_PORT"
grep -c "OLLAMA_MODEL" .env.example && echo "PASS: has OLLAMA_MODEL"
grep -c "OLLAMA_TIMEOUT" .env.example && echo "PASS: has OLLAMA_TIMEOUT"
grep -c "LLAMA_EXEC_TIMEOUT" .env.example && echo "PASS: has LLAMA_EXEC_TIMEOUT"
grep -c "LLAMA_MAX_OUTPUT" .env.example && echo "PASS: has LLAMA_MAX_OUTPUT"
```

## Expected output

```text
PASS: .env.example exists
PASS: has OLLAMA_HOST
PASS: has OLLAMA_PORT
PASS: has OLLAMA_MODEL
PASS: has OLLAMA_TIMEOUT
PASS: has LLAMA_EXEC_TIMEOUT
PASS: has LLAMA_MAX_OUTPUT
```

## Commit message

```text
chore: add .env.example configuration template
```
