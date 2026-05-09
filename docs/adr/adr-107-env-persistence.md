---
summary: Persist Model and Host Selection to .env
status: accepted
---

# ADR-107: Persist Model and Host Selection to .env

## Context

When users switch models via `/model`, the selection was only stored in runtime memory. Restarting the CLI reverted to the default model, forcing users to re-select every session. The `/scan` and `/color` commands already persisted to `.env` via a local `save_to_dotenv()` helper, but `/model` did not.

## Decision

1. Move `save_to_dotenv()` from a static function in `repl_commands.cpp` to a shared function in `config.h`/`config.cpp`.
2. After `/model` selection, persist `OLLAMA_MODEL`, `OLLAMA_HOST`, and `OLLAMA_PORT` to `.env`.
3. Show connection info (model, host, port, provider) in `/set` output.

## Consequences

- Model selection survives restarts without manual `.env` editing.
- Host selection (when picking a model from a remote host) is also persisted.
- `/set` gives a single view of all runtime settings including what's persisted.
- No new dependencies or config files introduced.
