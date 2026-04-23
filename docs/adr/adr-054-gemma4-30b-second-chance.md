# ADR-054: gemma4:30b — Second Chance with Tweaks

## Status

Proposed

## Date

2026-04-23

## Context

gemma4:26b is the current default model. It works but has clear limits:
hallucination on code reviews, context degradation after 3-4 iterations,
and inability to handle complex multi-file reasoning.

gemma4:30b (Q4_K_M quantization) offers better reasoning at the cost of
higher memory pressure. Previous attempts ran into performance issues on
Mac M2 hardware. This ADR documents the tweaks needed to make 30b viable.

ChatGPT analysis (chat.log, 2026-04-23) identified specific tuning
parameters that can make the difference between "unusable" and "usable."

## Decision

Trial gemma4:30b with the following configuration tweaks, evaluate against
concrete acceptance criteria, and decide whether to promote it to default
or keep 26b.

## Hardware Constraints (Mac M2)

| Resource | Available | 26b usage | 30b projected |
|----------|-----------|-----------|---------------|
| Unified memory | 16-24GB | ~17GB | ~18-20GB (Q4) |
| Memory pressure | Green | Green | Must stay green |
| Swap | None | None | Must remain zero |

**Hard rule**: if Activity Monitor shows yellow/orange memory pressure,
30b is not viable on this hardware. Fall back to 26b.

## Tweaks

### 1. Quantization — use Q4_K_M (highest impact)

```bash
# Pull the Q4 quantized variant
ollama pull gemma4:30b-q4_K_M
```

Q4_K_M is the sweet spot: ~15-18GB range, meaningful quality improvement
over 26b without exceeding memory budget.

| Quantization | Size | Quality | Viable on 16GB M2? |
|-------------|------|---------|---------------------|
| FP16 | ~60GB | Best | ❌ Impossible |
| Q8 | ~30GB | Great | ❌ Too large |
| Q4_K_M | ~18GB | Good | ⚠️ Tight but possible |
| Q3_K_M | ~15GB | Acceptable | ✅ Comfortable |

### 2. Context window — reduce to 4096 (high impact)

The KV-cache grows linearly with context size. At 30b parameters, each
extra 1K context costs significantly more memory than at 26b.

Create a Modelfile:

```text
FROM gemma4:30b-q4_K_M
PARAMETER num_ctx 4096
PARAMETER num_predict 2048
```

```bash
ollama create gemma4-30b-tuned -f Modelfile
```

| Setting | Default | Tuned | Why |
|---------|---------|-------|-----|
| num_ctx | 32768 | 4096 | Saves ~2-4GB RAM, matches effective sweet spot |
| num_predict | unlimited | 2048 | Prevents runaway generation eating context |

### 3. Thread limiting (medium impact)

```bash
export OLLAMA_NUM_THREADS=4
```

Too many threads on M2 causes memory thrashing and thermal throttling.
4 threads is the safe default; adjust up to 6 if memory pressure stays green.

### 4. Environment preparation (required)

Before starting a 30b session:

- Close Chrome, Electron apps, Docker, Xcode
- Check Activity Monitor → Memory Pressure = green
- Verify: `ollama ps` shows no other models loaded

### 5. System prompt stays the same

The existing system prompt (config.h) works for both 26b and 30b.
No changes needed — the discipline rules from ADR-050 apply equally.

## Acceptance Criteria

Run these tests to determine if 30b is viable:

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
OLLAMA_MODEL=gemma4-30b-tuned ./llama-cli

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

If 30b fails acceptance criteria:

1. Keep gemma4:26b as default
2. Use 30b only for specific "thinking" tasks (final review, complex reasoning)
3. Route per model-guide.md: simple tasks → e4b, medium → 26b, complex → cloud

## Model Routing Update

If 30b passes, update model-guide.md:

| Model | Role |
|-------|------|
| gemma4:e4b | Prompt execution, quick questions |
| gemma4:26b | General dev work, tool calling |
| gemma4-30b-tuned | Complex reasoning, longer analysis |
| kiro-cli / Gemini | Architecture, multi-file, reviews |

## References

- [ADR-044](adr-044-tidy-boilerplate.md) — Makefile leaf target rule
- [ADR-050](adr-050-reality-check-roadmap.md) — Reality check and usage discipline
- [model-guide.md](../model-guide.md) — Task-to-model routing
- [ChatGPT analysis](../../chat.log) — Hardware tuning recommendations
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
