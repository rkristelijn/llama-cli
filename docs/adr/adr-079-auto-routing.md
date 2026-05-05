# ADR-079: Auto-Routing Multi-Provider Architecture

*Status*: Accepted · *Date*: 2026-05-05

## Context

llama-cli now supports multiple providers (ollama, tgpt, gemini), multiple hosts (localhost, apsnlmac4050, jarvis, pepper), and multiple models per host. Users need both manual control (single provider/model) and automatic smart routing.

## Decision

### Two modes via .env or CLI

```bash
# Manual mode (default — backward compatible)
LLAMA_PROVIDER=ollama
OLLAMA_MODEL=qwen2.5-coder:14b

# Auto mode — smart routing across all providers and hosts
LLAMA_PROVIDER=auto
OLLAMA_MODEL=auto
```

When `LLAMA_PROVIDER=auto`, the system:

1. **Startup scan**: probes all configured hosts + checks tgpt/gemini availability
2. **Preflight validation**: before expensive calls, validates prompt isn't empty/nonsensical
3. **Tiered routing**: classifies prompt → routes to appropriate tier

### Routing tiers

| Tier | When | Target | Latency |
|------|------|--------|---------|
| 1: Fast local | Simple questions, math, factual | jarvis/pepper 3B | <1s |
| 2: Smart local | Code, explanation, medium complexity | apsnlmac4050 14B | 2-5s |
| 3: Power local | Architecture, creative, complex | apsnlmac4050 27B | 5-15s |
| 4: Cloud | Current info, rate-limited local, fallback | tgpt/gemini | 3-10s |

### Prompt classification (heuristic, no LLM call)

```
simple:  word_count < 30, no code markers, no complexity markers
medium:  word_count 30-90, or has question mark, or mild code markers
complex: word_count > 90, code keywords, multi-question, analysis requests
current: time-sensitive keywords ("today", "latest", "news", "current")
```

### Preflight validation

Before routing to Tier 3/4 (expensive), check:
- Prompt is not empty or just whitespace
- Prompt is not a repeat of the previous prompt
- Prompt has enough substance (>3 words)

If validation fails, respond locally with a clarification request.

### Structured delegation format

When the planner delegates to a sub-model, it uses:

```
Given: {context — compressed history or task description}
When: {the specific question or task}
Then: {expected output format and constraints}
```

This ensures consistent, parseable responses regardless of which model handles it.

### Multiline input

Linenoise doesn't support shift-enter (terminal limitation). Instead:
- `/m` toggles multiline mode (type freely, send with `/send` or empty line)
- Backslash `\` at end of line continues to next line

### REPL commands

| Command | Effect |
|---------|--------|
| `/auto` | Toggle auto-routing on/off |
| `/provider auto` | Same as LLAMA_PROVIDER=auto |
| `/provider ollama` | Force single provider |
| `/model auto` | Auto-select model per prompt |
| `/model qwen3:30b` | Force specific model |

### Startup output (auto mode)

```
Scanning... 
  ✓ apsnlmac4050:11434 — 9 models (qwen2.5-coder:14b, gemma4:26b, ...)
  ✓ jarvis:11434 — 1 model (llama3.2:3b)
  ✓ pepper:11434 — 2 models (llama3.2:3b, gemma4:e4b)
  ✓ localhost:11434 — 3 models (gemma2:2b, llama3.2:3b, phi3)
  ✓ tgpt — ready (cloud, free)
  ✓ gemini — ready (cloud, free)
Auto-routing enabled: 15 models across 4 hosts + 2 cloud providers.
```

## Consequences

- Backward compatible: default behavior unchanged (single provider/model)
- `LLAMA_PROVIDER=auto` + `OLLAMA_MODEL=auto` enables full smart routing
- Startup is ~2s slower in auto mode (network scan)
- All routing decisions logged with host/provider/model in events.jsonl
- Preflight prevents wasted tokens on bad prompts
- Structured delegation format enables future multi-agent workflows
