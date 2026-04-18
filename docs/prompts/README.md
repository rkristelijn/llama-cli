# AI Agent Task Prompts

Self-contained prompts for executing development tasks with a local Ollama model.

## How to use

1. Pick a prompt file from this directory
2. Copy the entire content
3. Paste it into your local LLM (e.g., `llama-cli`, Ollama, or any chat interface)
4. The model will output exact code to create/modify
5. Apply the changes
6. Run the verification command at the bottom of the prompt
7. If verification passes, commit with the suggested commit message

## Format

Every prompt contains:
- **Context**: what files exist, what they currently contain
- **Task**: exactly what to create or change
- **Code**: copy-paste ready code blocks
- **Verify**: exact shell command to confirm success
- **Expected**: what the verification output should look like
- **Commit**: the conventional commit message to use

## Current prompts

### Sprint 1 — CMMI 0 (Essentials)

| Prompt | CMMI Check | Status |
|--------|-----------|--------|
| [01-commit-msg-hook.md](01-commit-msg-hook.md) | 0.1 Conventional commits | ⬜ |
| [02-branch-naming.md](02-branch-naming.md) | 0.2 Branch naming | ⬜ |
| [03-env-example.md](03-env-example.md) | 0.8 Onboarding | ⬜ |
| [04-changelog.md](04-changelog.md) | 0.8 Release tracking | ⬜ |

### Sprint 2 — CMMI 1 (Managed) + Streaming

| Prompt | CMMI Check | Status |
|--------|-----------|--------|
| [05-coverage-bump.md](05-coverage-bump.md) | 1.1 Coverage ≥ 60% | ⬜ |
| [06-todo-scraping.md](06-todo-scraping.md) | 1.3 Tech debt log | ⬜ |
| [07-branch-protection.md](07-branch-protection.md) | 1.6 Peer review | ⬜ |
| [08-doc-change-ci.md](08-doc-change-ci.md) | 1.8 Doc enforcement | ⬜ |
| [09-streaming-ollama.md](09-streaming-ollama.md) | Feature: streaming | ⬜ |
| [10-streaming-repl.md](10-streaming-repl.md) | Feature: streaming | ⬜ |
| [11-streaming-tests.md](11-streaming-tests.md) | Feature: streaming | ⬜ |
