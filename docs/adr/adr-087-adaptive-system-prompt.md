# ADR-087: Adaptive System Prompt

## Status

Accepted

## Context

The system prompt in REPL mode includes the base prompt (~800 tokens), `.kiro/agents/` prompt extensions, and all resource files (README, CONTRIBUTING, Makefile, TODO, INDEX, lessons.yml). This totals ~20,000 tokens.

Local models like `qwen2.5-coder:14b` have limited context windows (typically 8k-32k). When the system prompt consumes 20k tokens, there's almost no room for the user's question and the model's response. The result: incoherent or minimal answers.

Cloud models handle this fine (128k+ context), but llama-cli targets local models where context is precious.

## Decision

### 1. System prompt budget

The system prompt must not exceed **25% of the model's context window** (or a hard cap of 4096 tokens, whichever is smaller). This leaves 75%+ for conversation history and responses.

### 2. Layered prompt structure

| Layer | ~Tokens | Content | When included |
|-------|---------|---------|---------------|
| Core | 200 | Language, style, conciseness | Always |
| Tools | 400 | exec, read, write, str_replace, mermaid syntax | Always (REPL mode) |
| Project | 300 | One-line project summary + key make targets | If budget allows |
| Resources | 5000+ | Full file contents from .kiro/agents/ | Only if budget allows (rarely) |

### 3. Resource loading policy

`.kiro/agents/*.json` resources (README, Makefile, etc.) are **not** loaded into the system prompt by default. Instead:

- The agent prompt text is appended (compact instructions)
- Resource files are available on-demand via `<read>` annotations
- The model is instructed to read files when it needs them

### 4. Context window detection

On model switch, query `/api/show` for `num_ctx` to determine the budget. Fallback: assume 8192 if unknown.

### 5. Tokens/sec tracking (future)

After each response, compute actual tokens/sec from Ollama's `eval_count` / `eval_duration`. Store in `.tmp/model-stats.json` as rolling average. Display in `/model` list (replacing static estimates).

## Consequences

- Immediate: local models give coherent answers instead of garbage
- The `.kiro/agents/` resource loading becomes opt-in (large models) rather than always-on
- Models that support tool use will read files on demand — same capability, less upfront cost
- `/usage` command can show prompt budget utilization
