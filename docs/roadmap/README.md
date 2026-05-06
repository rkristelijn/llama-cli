# Roadmap

> **Note**: This roadmap previously linked to `docs/backlog/` items which have been consolidated into ADRs. See [docs/adr/README.md](../adr/README.md) for the current list of proposed features. The main [README.md](../../README.md) roadmap section is the canonical source.

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
| Log all REPL events (user input, dispatch type) | — | Todo |
| Log exec events (`!`, `!!`, `<exec>`) with timing | — | Todo |
| Log file operations (`<write>`, `<str_replace>`, `<read>`) | — | Todo |
| Log token counts per LLM call (already in ollama.cpp) | — | Done |
| Log session start/end with model info | — | Todo |
| Build `make log-stats` — aggregate token usage, session duration, command frequency | — | Todo |

**Outcome**: You can answer "how many tokens did I burn today?" and "which prompts are expensive?"

## Phase 2: Prompt efficiency (spend tokens wisely)

*Research shows optimized prompts reduce inference time by up to 60%.*

| Task | Backlog | Status |
|------|---------|--------|
| Context compression — summarize old messages, keep last 3 exchanges | — | Todo |
| Temperature tuning — low for code (0.1), medium for analysis (0.3), higher for creative (0.8) | — | Todo |
| Prompt templates — CRISP framework for structured prompts | — | Todo |
| System prompt optimization — shorter, more directive, task-aware | — | Todo |
| Truncate large exec output before feeding back (first/last N lines) | — | Todo |

**Outcome**: Same quality, fewer tokens, faster responses.

## Phase 3: Daily driver features (make it indispensable)

| Task | Backlog | Status |
|------|---------|--------|
| Streaming responses — see tokens as they arrive | — | Todo |
| Smart confirmation — copy, amend, redirect on y/n prompt | — | Todo |
| Wingman: command tips after `!!` | — | Todo |
| Wingman: prompt preflight check | — | Todo |
| Diff preview before file overwrite | — | Todo |
| Command permissions (auto-allow safe commands) | — | Todo |
| Runtime model switching | — | Todo |

**Outcome**: A tool you reach for first, before opening a browser or paying for API calls.

## Phase 4: Architecture (scale up)

| Task | Backlog | Status |
|------|---------|--------|
| Provider abstraction | — | Todo |
| Planner/executor separation | — | Todo |
| Execution sandbox | — | Todo |
| Multi-agent support | — | Todo |
| Distributed Ollama | — | Todo |

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
| Commit message validation | — | Todo |
| Branch naming validation | — | Todo |
| Issue and PR templates | — | Todo |
| CHANGELOG.md | — | Todo |
| Fix `make setup` (missing tools) | — | Todo |

### CMMI 1 — Managed

| Task | Backlog | Status |
|------|---------|--------|
| Coverage bump 55% → 60% | — | Todo |
| TODO scraping → TECHDEBT.md | — | Todo |
| Branch protection (peer review) | — | Todo |
| Doc-change CI enforcement | — | Todo |
| Reduce complexity in repl.cpp | — | Todo |
