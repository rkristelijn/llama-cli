# Roadmap

The goal: make llama-cli a **daily driver** — fast, smart with resources, and reliable enough to replace expensive cloud models for most tasks.

## Current state

- ✅ Logger exists (`src/logging/logger.cpp`) — writes JSONL to `~/.llama-cli/events.jsonl`
- ✅ Only Ollama provider logs events (6 LOG_EVENT calls in `ollama.cpp`)
- ❌ REPL, exec, file writes, str_replace — none of these log events
- ❌ No log analysis tooling beyond `scripts/dev/log-viewer.sh`
- ❌ No prompt optimization — system prompt is static, no context compression
- ❌ No temperature/parameter tuning per task type

## Phase 1: Observability (see what's happening)

*Before optimizing, you need to measure.*

| Task | Backlog | Status |
|------|---------|--------|
| Log all REPL events (user input, dispatch type) | [001](../backlog/001-log-optimization.md) | Todo |
| Log exec events (`!`, `!!`, `<exec>`) with timing | [001](../backlog/001-log-optimization.md) | Todo |
| Log file operations (`<write>`, `<str_replace>`, `<read>`) | [001](../backlog/001-log-optimization.md) | Todo |
| Log token counts per LLM call (already in ollama.cpp) | [001](../backlog/001-log-optimization.md) | Done |
| Log session start/end with model info | [001](../backlog/001-log-optimization.md) | Todo |
| Build `make log-stats` — aggregate token usage, session duration, command frequency | [001](../backlog/001-log-optimization.md) | Todo |

**Outcome**: You can answer "how many tokens did I burn today?" and "which prompts are expensive?"

## Phase 2: Prompt efficiency (spend tokens wisely)

*Research shows optimized prompts reduce inference time by up to 60%.*

| Task | Backlog | Status |
|------|---------|--------|
| Context compression — summarize old messages, keep last 3 exchanges | [019](../backlog/019-context-compression.md) | Todo |
| Temperature tuning — low for code (0.1), medium for analysis (0.3), higher for creative (0.8) | [020](../backlog/020-temperature-tuning.md) | Todo |
| Prompt templates — CRISP framework for structured prompts | [021](../backlog/021-prompt-templates.md) | Todo |
| System prompt optimization — shorter, more directive, task-aware | [007](../backlog/007-exec-output-tuning.md) | Todo |
| Truncate large exec output before feeding back (first/last N lines) | [007](../backlog/007-exec-output-tuning.md) | Todo |

**Outcome**: Same quality, fewer tokens, faster responses.

## Phase 3: Daily driver features (make it indispensable)

| Task | Backlog | Status |
|------|---------|--------|
| Streaming responses — see tokens as they arrive | [005](../backlog/005-streaming.md) | Todo |
| Smart confirmation — copy, amend, redirect on y/n prompt | [004](../backlog/004-smart-confirmation.md) | Todo |
| Wingman: command tips after `!!` | [002](../backlog/002-wingman-command-tips.md) | Todo |
| Wingman: prompt preflight check | [003](../backlog/003-wingman-preflight.md) | Todo |
| Diff preview before file overwrite | [013](../backlog/013-diff-preview.md) | Todo |
| Command permissions (auto-allow safe commands) | [010](../backlog/010-command-permissions.md) | Todo |
| Runtime model switching | [011](../backlog/011-model-switching.md) | Todo |

**Outcome**: A tool you reach for first, before opening a browser or paying for API calls.

## Phase 4: Architecture (scale up)

| Task | Backlog | Status |
|------|---------|--------|
| Provider abstraction | [014](../backlog/014-provider-abstraction.md) | Todo |
| Planner/executor separation | [015](../backlog/015-planner-executor.md) | Todo |
| Execution sandbox | [016](../backlog/016-execution-sandbox.md) | Todo |
| Multi-agent support | [017](../backlog/017-multi-agent.md) | Todo |
| Distributed Ollama | [006](../backlog/006-distributed-ollama.md) | Todo |

## Principles

1. **Measure first** — no optimization without data
2. **Local first** — everything runs on your machine, no cloud dependency
3. **Token-aware** — every prompt should justify its token cost
4. **Non-breaking** — each phase delivers value independently

## Quality track: CMMI 0 → 1

Parallel to the feature phases above. See [ADR-048](../adr/adr-048-quality-framework.md) for the full framework.

### CMMI 0 — Essentials (current: 5/8 pass)

| Task | Backlog | Status |
|------|---------|--------|
| Commit message validation | [023](../backlog/023-commit-msg-hook.md) | Todo |
| Branch naming validation | [024](../backlog/024-branch-naming.md) | Todo |
| Issue and PR templates | [025](../backlog/025-issue-pr-templates.md) | Todo |
| CHANGELOG.md | [026](../backlog/026-changelog.md) | Todo |
| Fix `make setup` (missing tools) | [022](../backlog/022-fix-make-setup.md) | Todo |

### CMMI 1 — Managed

| Task | Backlog | Status |
|------|---------|--------|
| Coverage bump 55% → 60% | [027](../backlog/027-coverage-bump.md) | Todo |
| TODO scraping → TECHDEBT.md | [028](../backlog/028-todo-scraping.md) | Todo |
| Branch protection (peer review) | [029](../backlog/029-branch-protection.md) | Todo |
| Doc-change CI enforcement | [030](../backlog/030-doc-change-ci.md) | Todo |
| Reduce complexity in repl.cpp | [018](../backlog/018-reduce-complexity.md) | Todo |
