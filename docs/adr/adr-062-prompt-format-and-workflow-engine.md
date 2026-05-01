# ADR-062: Context-First Architecture & Workflow Engine

## Status

Proposed

## Context

After evaluating the competitive landscape (Codex CLI, Copilot CLI, Open WebUI, etc.), llama-cli risks being "just another Ollama wrapper" if it stays at the transport layer. The 2025–2026 shift moved value up the stack — from model access to workflow integration.

Key insight from analysis: **the winning pattern is not "talk to models" but "solve developer workflows"**.

### Current position

llama-cli is model-first: send prompt → choose model → get answer. This is useful but replaceable — Ollama CLI already exists, Codex CLI does more.

### Opportunity

No tool fully owns the "context-controlled, Unix-composable, local-first AI CLI" space. The gap is:

- Explicit context management (what did the model see?)
- Reproducibility (can I replay this?)
- Composability (pipes, not prefixes)

## Decision

Shift llama-cli from **model-first** to **context-first** design. Models become execution engines; context becomes the primary abstraction.

### Core principles

1. **Context is a first-class object** — not a side effect of prompts
2. **Explicit over implicit** — user controls what the model sees
3. **Composable** — works with Unix pipes and standard I/O
4. **Smart routing** — model selection based on task complexity, not manual prefixes
5. **Reproducible** — sessions can be saved, replayed, shared

### Proposed CLI extensions

```bash
# Context management
llama-cli ctx add file.ts          # add file to context
git diff | llama-cli ctx add       # pipe into context
llama-cli ctx list                 # show current context
llama-cli ctx clear                # reset context
llama-cli ctx snapshot save bug-1  # save for later

# Execution with context capture
llama-cli exec "npm test"          # run + capture output as context

# Ask with accumulated context
llama-cli ask "fix the failing test"

# Provider routing (automatic by default)
llama-cli ask "simple question"           # → local (Ollama)
llama-cli ask "complex refactor"          # → cloud (if configured)
llama-cli ask --provider=kiro "deep task" # explicit override
```

### Complexity scoring for smart routing

Three dimensions determine where a prompt should go:

| Dimension | Weight | Low | High |
|-----------|--------|-----|------|
| Context size (tokens/files) | 40% | single file | multi-file project |
| Task type | 40% | summarize | architecture/agentic |
| Iteration depth | 20% | one-shot | multi-step reasoning |

Local models handle: summarization, explanation, simple edits, formatting.
Cloud escalation for: multi-file refactors, architecture decisions, complex debugging.

### Preflight tips (before cloud escalation)

Before reaching out to an expensive provider, the tool can:

1. Compress context (remove irrelevant files)
2. Suggest narrowing the question
3. Show estimated token cost
4. Offer to try local first with a simpler prompt

## Alternatives considered

| Approach | Power | Control | UX | Verdict |
|----------|-------|---------|-----|---------|
| Prefix model (`ollama:`, `kiro:`) | Low | High | Clunky | Rejected — doesn't scale |
| Full agent (Codex-style) | Very high | Low | Magic | Out of scope — different product |
| Context pipeline (this ADR) | High | High | Clean | **Selected** |

## Consequences

### Positive

- Defensible position — solves something unsolved (context visibility + control)
- Unix-native — works with existing developer workflows
- Incremental — can be added on top of current llama-cli without breaking changes
- Enterprise-useful — reproducible, auditable AI interactions

### Negative

- More complex implementation than current simple REPL
- Context storage needs design (memory, disk, format)
- Smart routing requires heuristics that may frustrate power users

### Migration path

1. Keep existing `llama-cli ask` / REPL as-is
2. Add `ctx` subcommand for explicit context management
3. Add `exec` with context capture
4. Add complexity scoring + provider routing
5. Add snapshot/replay for reproducibility

## References

- [ADR-050: Reality Check — Positioning and Roadmap](adr-050-reality-check-roadmap.md)
- [ADR-020: Provider Abstraction Layer](adr-020-provider-abstraction.md)
- [Codex CLI](https://github.com/openai/codex) — agent-style competitor
- [freshbrewed.science/2025/08/22/codex.html](https://freshbrewed.science/2025/08/22/codex.html) — landscape analysis
