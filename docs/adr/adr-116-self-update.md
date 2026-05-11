---
summary: Self-Update Command
status: accepted
---

# ADR-116: Self-Update Command (`/update`)

## Context

Users install llama-cli via `curl | bash` or manual download. There's no built-in way to check for or install updates without re-running the install script.

## Decision

Add `/update` REPL command and `--update` CLI flag that:

1. Queries GitHub Releases API for the latest version
2. Compares with current `LLAMA_CLI_VERSION`
3. If newer: downloads platform-appropriate binary, verifies SHA256, replaces self
4. If current: prints "already up to date"

### Implementation

- Uses existing `exec` module for curl/download
- Binary detection: `uname -s` + `uname -m` → `llama-cli-{os}-{arch}`
- Checksum: download `.sha256` file, verify before replacing
- Atomic replace: write to temp, verify, `mv` over current binary
- No external dependencies beyond curl (already required for Ollama)

### Safety

- Requires write permission to binary location
- Shows version diff before applying (user confirms)
- Falls back to install.sh URL if direct download fails

## Consequences

- Users can update without leaving the REPL
- Version drift is visible and actionable
- No package manager dependency
