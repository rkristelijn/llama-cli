---
summary: Local AI Code Review via /review Command
status: accepted
---

# ADR-113: Local AI Code Review via /review Command

## Context

CodeRabbit provides excellent AI-powered code reviews on GitHub PRs, but:

- It only works on pushed PRs (not local changes)
- It requires internet and a CodeRabbit account
- Free tier is rate-limited (3 reviews/hour)
- Code leaves your machine

We already have `scripts/gh/pr-feedback.sh` to download CodeRabbit comments, but there's no way to get a code review *before* pushing — locally, privately, and for free.

Since llama-cli already has an LLM connection, exec capabilities, and markdown rendering, we can offer CodeRabbit-style reviews locally in any git repo.

## Decision

Add a `/review` REPL command that:

1. Runs `git diff` (or variant) to capture changes
2. Sends the diff to the active LLM with a code review system prompt
3. Streams the review back with markdown rendering

### Variants

| Command | Git command | Use case |
|---------|-------------|----------|
| `/review` | `git diff` | Review uncommitted changes |
| `/review staged` | `git diff --cached` | Review what's about to be committed |
| `/review branch` | `git diff main...HEAD` | Review entire branch |
| `/review <path>` | `git diff -- <path>` | Review specific file |

### System Prompt

The review prompt asks the LLM to act as a senior code reviewer:

```text
You are a senior code reviewer. Analyze this git diff and provide a concise review.
Focus on: bugs, security issues, performance problems, and readability.
Format: use markdown with ## sections for Summary, Issues (if any), and Suggestions.
Be specific — reference file names and line numbers from the diff.
If the code looks good, say so briefly.
```

### Implementation

- Add `handle_review()` in `repl_commands.cpp` (same pattern as other handlers)
- Use `cmd_exec()` to run git diff with the configured exec timeout
- Stream the response via `s.stream_chat()` for real-time output
- Render with existing markdown renderer

### Diff Size Limits

If the diff exceeds 8000 characters (roughly 2000 tokens), truncate with a warning. Large diffs should use `/review <file>` for focused review.

## Consequences

- **Free**: uses whatever model is active — no API keys, no rate limits
- **Private**: code never leaves the machine
- **Portable**: works in any git repo where llama-cli runs
- **Quality depends on model**: larger models (26B+) give better reviews than small ones
- **No persistence**: reviews are ephemeral (in chat history only, saveable via `/chat save`)

## Alternatives Considered

- **Wrap CodeRabbit CLI**: requires install, account, internet, rate limits
- **Dedicated review agent**: over-engineered for v1 — a single command is simpler
- **Background review on every commit**: too invasive, user should opt-in
