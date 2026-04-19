# ADR-050: Reality Check — Positioning and Roadmap

*Status*: Accepted · *Date*: 2026-04-19 · *Supersedes*: earlier draft (ChatGPT-generated)

## Context

This ADR replaces an earlier draft that recommended pivoting llama-cli into a "quality enforcement engine." That advice was based on a surface-level reading of the project and missed what already exists. This version provides an honest assessment grounded in the actual codebase.

## What llama-cli Actually Is

A **local-first AI terminal assistant** built in C++. It connects to Ollama, runs entirely offline, and provides:

- Interactive chat with conversation memory
- File operations: write, str_replace (targeted edits), read (with line ranges and search)
- Command execution with user confirmation (`!`, `!!`, `<exec>`)
- Diff preview before file writes
- JSONL event logging (audit trail)
- Runtime model switching
- Stdin pipe support for scripting
- TUI with ANSI colors, markdown rendering, spinner
- Arrow key history (linenoise)
- Quality gates via git hooks (pre-commit: 6 checks, pre-push: 15 checks)

This is not "another AI CLI with guidelines." This is a working tool with real enforcement.

## Competitor Landscape

| Tool | Language | Offline | Free | File Edit | Exec | Audit Log | Diff Preview | Streaming |
|------|----------|---------|------|-----------|------|-----------|--------------|-----------|
| **llama-cli** | C++ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Claude Code | TypeScript | ❌ | ❌ | ✅ | ✅ | ❌ | ✅ | ✅ |
| Kiro CLI | — | ✅¹ | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ |
| aider | Python | ✅¹ | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ |
| Copilot CLI | — | ❌ | ❌² | ✅ | ✅ | ❌ | ✅ | ✅ |
| Codex CLI | — | ❌ | ❌ | ✅ | ✅ | ❌ | ✅ | ✅ |
| llm | Python | ✅¹ | ✅ | ❌ | ❌ | ❌ | ❌ | ✅ |
| Continue.dev | TypeScript | ✅¹ | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ |
| Cline | TypeScript | ✅¹ | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ |

¹ Via Ollama or local model provider  
² Free tier with limits, $10/month for full access

### Key Observations

1. **Most tools require cloud APIs or API keys.** llama-cli is one of the few that works fully offline out of the box with zero configuration beyond having Ollama running.

2. **No competitor has built-in audit logging.** llama-cli's JSONL event log is unique — every LLM call, file write, command execution, and user decision is recorded.

3. **Streaming is the biggest gap.** Every competitor has it. llama-cli does not. This is the most visible UX difference.

4. **The C++ choice is unusual** but gives real advantages: fast startup, no runtime dependencies, single binary distribution.

## What llama-cli Is NOT

- Not a quality enforcement engine (that's the internal dev process, not the product)
- Not a package manager
- Not a replacement for linters
- Not an IDE extension

The quality framework (ADR-048) is valuable as an **internal development practice**. It makes this project better. It is not the product itself.

## Honest Assessment

### Strengths

- **Fully offline, zero cloud** — no API keys, no subscriptions, no data leaving the machine
- **Single binary** — curl install, works immediately
- **Audit trail** — unique among competitors
- **Confirmation workflow** — every file write and command requires user approval
- **Quality gates** — 15+ automated checks on every push

### Weaknesses

- **No streaming** — the biggest UX gap vs every competitor
- **Small model limitation** — local models are less capable than cloud APIs
- **Solo maintainer** — bus factor of 1
- **C++ contributor barrier** — fewer potential contributors than Python/TypeScript projects
- **No plugin system** — not extensible by users

### Honest Position

- Useful for developers who want privacy-first local AI assistance
- Not competing with Claude Code or Copilot on capability (cloud models are stronger)
- Competing on **privacy, speed, simplicity, and zero dependencies**

## Prioritized Roadmap

Based on what matters most for usability and differentiation:

### Priority 1 — Core UX (do first)

These close the biggest gaps with competitors:

| # | Feature | Why | Backlog |
|---|---------|-----|---------|
| 1 | Streaming responses | Every competitor has this; biggest UX gap | [005](../backlog/005-streaming.md) |
| 2 | Inline code rendering | Markdown output looks broken without it | [031](../backlog/031-inline-code-rendering.md) |
| 3 | Fix release pipeline | Can't ship reliably without this | [034](../backlog/034-fix-release.md) |
| 4 | Tab autocompletion | Basic CLI ergonomics | [033](../backlog/033-tab-autocompletion.md) |

### Priority 2 — Developer Experience

Make the tool pleasant to use daily:

| # | Feature | Why | Backlog |
|---|---------|-----|---------|
| 5 | Context compression | Long conversations break with local models | [019](../backlog/019-context-compression.md) |
| 6 | Prompt templates | Consistent results for common tasks | [021](../backlog/021-prompt-templates.md) |
| 7 | Exec output tuning | Command output floods context | [007](../backlog/007-exec-output-tuning.md) |
| 8 | Smart confirmation | Copy, amend, redirect proposed actions | [004](../backlog/004-smart-confirmation.md) |

### Priority 3 — Robustness

Harden what exists:

| # | Feature | Why | Backlog |
|---|---------|-----|---------|
| 9 | Execution sandbox | Safety for LLM-proposed commands | [016](../backlog/016-execution-sandbox.md) |
| 10 | Command permissions | Whitelist/blacklist for exec | [010](../backlog/010-command-permissions.md) |
| 11 | Coverage bump 55→60% | Catch regressions | [027](../backlog/027-coverage-bump.md) |
| 12 | Reduce complexity | Keep codebase maintainable | [018](../backlog/018-reduce-complexity.md) |

### Priority 4 — Future Differentiation

Only after priorities 1–3 are solid:

| # | Feature | Why | Backlog |
|---|---------|-----|---------|
| 13 | Provider abstraction | Support non-Ollama backends | [014](../backlog/014-provider-abstraction.md) |
| 14 | Planner/executor | Multi-step task execution | [015](../backlog/015-planner-executor.md) |
| 15 | Distributed Ollama | Use remote GPU machines | [006](../backlog/006-distributed-ollama.md) |
| 16 | Multi-agent | Parallel task execution | [017](../backlog/017-multi-agent.md) |

### Deprioritized

These are nice-to-have but not differentiating:

- Mermaid rendering ([032](../backlog/032-mermaid-rendering.md)) — niche
- Temperature tuning ([020](../backlog/020-temperature-tuning.md)) — marginal impact
- Nick/custom prompt ([012](../backlog/012-nick-prompt.md)) — cosmetic
- Project rename ([008](../backlog/008-project-rename.md)) — disruptive, low value now

## Guiding Principle

> **Make the local AI assistant work so well that the cloud alternative isn't worth the privacy trade-off.**

Don't pivot. Don't chase features that cloud tools do better. Double down on what they can't do: **offline, fast, private, auditable.**

## References

- [ADR-048](adr-048-quality-framework.md) — Quality framework (internal dev practice)
- [ADR-027](adr-027-event-logging.md) — Event logging design
- [Backlog](../backlog/README.md) — Full feature backlog
- [README roadmap](../../README.md#roadmap) — Public roadmap
