# ADR-054: gemma4:31b — Second Chance with Tweaks

## Status

Accepted

Proposed

## Date

2026-04-23

## Context

gemma4:26b is the current default model. It works but has clear limits:
hallucination on code reviews, context degradation after 3-4 iterations,
and inability to handle complex multi-file reasoning.

gemma4:31b (30.7B dense parameters, Q4 quantized at 20GB) offers better
reasoning at the cost of higher memory pressure. This ADR documents the
tweaks needed to make 31b viable on Mac M2 hardware.

ChatGPT analysis (chat.log, 2026-04-23) identified specific tuning
parameters that can make the difference between "unusable" and "usable."

## Decision

Trial gemma4:31b with reduced context window and generation limit,
evaluate against concrete acceptance criteria, and decide whether to
promote it to default or keep 26b.

## Hardware Constraints (Mac M2)

| Resource | Available | 26b usage | 31b projected |
|----------|-----------|-----------|---------------|
| Unified memory | 16-24GB | ~18GB | ~20GB |
| Memory pressure | Green | Green | Must stay green |
| Swap | None | None | Must remain zero |

**Hard rule**: if Activity Monitor shows yellow/orange memory pressure,
31b is not viable on this hardware. Fall back to 26b.

## Tweaks

### 1. Quantization — default tag is already Q4 (no action needed)

The `gemma4:31b` tag on Ollama is already Q4 quantized at 20GB.
No separate Q4_K_M variant needed.

```bash
ollama pull gemma4:31b
```

### 2. Context window — reduce to 4096 (high impact)

The KV-cache grows linearly with context size. At 31b parameters, each
extra 1K context costs significantly more memory than at 26b.

Create a Modelfile:

```text
FROM gemma4:31b
PARAMETER num_ctx 4096
PARAMETER num_predict 2048
```

```bash
ollama create gemma4-31b-tuned -f Modelfile
```

| Setting | Default | Tuned | Why |
|---------|---------|-------|-----|
| num_ctx | 256K | 4096 | Saves GB of KV-cache RAM |
| num_predict | unlimited | 2048 | Prevents runaway generation eating context |

### 3. Thread limiting (medium impact)

```bash
export OLLAMA_NUM_THREADS=4
```

Too many threads on M2 causes memory thrashing and thermal throttling.
4 threads is the safe default; adjust up to 6 if memory pressure stays green.

### 4. Environment preparation (required)

Before starting a 31b session:

- Close Chrome, Electron apps, Docker, Xcode
- Check Activity Monitor → Memory Pressure = green
- Verify: `ollama ps` shows no other models loaded

### 5. System prompt stays the same

The existing system prompt (config.h) works for both 26b and 31b.
No changes needed — the discipline rules from ADR-050 apply equally.

## Acceptance Criteria

Run these tests to determine if 31b is viable:

| Test | Pass criteria |
|------|---------------|
| Memory pressure | Stays green throughout session |
| First token latency | < 5 seconds |
| Tokens/sec | ≥ 5 tok/s sustained |
| `make check` after model edit | All checks pass |
| 4-iteration code task | No hallucination, correct output |
| `!!cat` + reasoning | Correctly analyzes loaded file content |

### Test script

```bash
# 1. Start with tuned model
OLLAMA_MODEL=gemma4-31b-tuned ./llama-cli

# 2. Basic reasoning
> wat is 1+1

# 3. File reading + analysis
> !!cat src/main.cpp
> hoeveel functies staan hierin?

# 4. Code edit task
> voeg een comment toe boven de main functie in src/main.cpp

# 5. Check latency in trace output
# [TRACE] 200 Xms — X should be < 30000 for simple tasks
```

## Fallback

If 31b fails acceptance criteria:

1. Keep gemma4:26b as default
2. Use 31b only for specific "thinking" tasks (final review, complex reasoning)
3. Route per model-guide.md: simple tasks → e4b, medium → 26b, complex → cloud

## Model Routing Update

If 31b passes, update model-guide.md:

| Model | Role |
|-------|------|
| gemma4:e4b | Prompt execution, quick questions |
| gemma4:26b | General dev work, tool calling |
| gemma4-31b-tuned | Complex reasoning, longer analysis |
| kiro-cli / Gemini | Architecture, multi-file, reviews |

## References

- [ADR-044](adr-044-tidy-boilerplate.md) — Makefile leaf target rule
- [ADR-050](adr-050-reality-check-roadmap.md) — Reality check and usage discipline
- [model-guide.md](../model-guide.md) — Task-to-model routing
- ChatGPT analysis — Hardware tuning recommendations
- [Ollama Modelfile docs](https://github.com/ollama/ollama/blob/main/docs/modelfile.md)

## Addendum (2026-04-23): Implemented Tweaks

### Reminder nudge injection

After iteration 2, a short system message is injected into the conversation
history to prevent model drift. This is standard practice in agent systems
(Claude Code, aider, Cursor all do variants of this).

```text
history: [system prompt] [user] [assistant] [user] [assistant] [system: reminder] [user] ...
```

Design choices:

- Injected as a `system` role message, not as a user message prefix — keeps
  user input clean and avoids breaking mock/echo tests
- Only after iteration 2 — the first two turns rarely drift
- Kept to one sentence (~20 tokens) to minimize context cost
- Content focuses on the two biggest failure modes observed in chat.log:
  hallucination about unread code, and scores without criteria

### Anti-hallucination system prompt

Added to the default system prompt in `config.h`:

> Never claim code contains specific patterns, functions, or design choices
> unless you have read the actual file content in this conversation. If you
> have not read a file, say so instead of guessing. Do not give scores or
> ratings without measurable criteria.

### gcda cleanup in test runner

Coverage profiling data (`.gcda` files) becomes stale after recompilation,
causing hundreds of "corrupt arc tag" warnings that mask real test failures.
`scripts/test/run-unit.sh` now deletes gcda files between the build loop
and the run loop.
