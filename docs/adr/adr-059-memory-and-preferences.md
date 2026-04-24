# ADR-059: Persistent Memory & Preferences

## Status

Proposed

## Context

llama-cli starts every session with a blank slate — the LLM knows nothing about the user, their project, or their preferences. Users repeat the same context ("I work in C++", "answer in Dutch") every session. This friction reduces usability compared to cloud assistants that remember context across sessions.

We need a lightweight, local-first mechanism for the LLM to retain facts and style preferences across sessions without compromising privacy.

## Decision

Introduce two persistent stores, both loaded into the system prompt at startup:

### Memory — facts the LLM should know

- **File**: `.cache/llama-memory.md`
- **Purpose**: accumulated facts about the user, project, and environment
- **Format**: plain Markdown (one fact per line, `-` prefix)
- **Growth**: append-only via `/mem add`, clearable via `/mem clear`
- **Example content**:

  ```markdown
  - Remi works with C++ on macOS
  - Project uses Ollama with gemma4:26b
  - Preferred language: Dutch
  ```

### Preferences — how the LLM should behave

- **File**: `.preferences/style.md`
- **Purpose**: stable style and behavior instructions
- **Format**: plain Markdown (human-editable, LLM-readable)
- **Growth**: via `/pref add` or manual editing
- **Example content**:

  ```markdown
  - Always respond in the same language as the user
  - Be concise, no emoji
  - Use code examples over explanations
  ```

### REPL Commands

```text
/mem                  — show all memories
/mem add <fact>       — append a fact
/mem clear            — wipe all memories

/pref                 — show all preferences
/pref add <pref>      — append a preference
/pref clear           — wipe all preferences
```

### Loading at Startup

Both files are read in `run_repl()` (or `main()` alongside `.kiro/agents/`) and prepended to the system prompt:

```text
[system prompt] + [preferences] + [memory]
```

Preferences come before memory because they define *how* to respond, while memory provides *what* to know. This mirrors the existing pattern where `.kiro/agents/*.json` prompts are appended to the system prompt.

### Storage Location

| Store | Path | Gitignored | Why |
|-------|------|------------|-----|
| Memory | `.cache/llama-memory.md` | Yes (`.cache/` already in `.gitignore`) | Personal, session-accumulated |
| Preferences | `.preferences/style.md` | Yes (add to `.gitignore`) | Personal style choices |

Both are repo-local (not `~/.llama-cli/`) so different projects can have different context. Users who want shared memory across projects can symlink.

### File Format

Plain Markdown with `-` list items. Chosen over JSON because:

- Human-readable and editable in any editor
- LLM-native — models understand Markdown natively
- No parsing library needed — just read and inject as text
- Grep-friendly for debugging

## Implementation

### Files to modify

1. **`src/config/config.h`** — add `memory_path` and `preferences_path` fields
2. **`src/repl/repl.cpp`** — load files at startup, add `/mem` and `/pref` command handlers
3. **`src/command/command.cpp`** — register new slash commands in parser
4. **`.gitignore`** — add `.preferences/`

### Estimated effort

~80 lines of code. The pattern already exists (`.kiro/agents/` loading, `/set` subcommands).

## Consequences

### Positive

- **Better UX**: LLM remembers context across sessions without cloud dependency
- **Privacy-first**: all data stays local, gitignored, never transmitted
- **Human-editable**: users can directly edit the Markdown files
- **Minimal complexity**: plain file I/O, no database, no new dependencies

### Negative

- **Token cost**: memory and preferences consume system prompt tokens every turn
- **Staleness**: memory can accumulate outdated facts — user must `/mem clear` periodically
- **No auto-learning**: the LLM cannot add memories itself (by design — user controls what is stored)
