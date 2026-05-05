# ADR-078: Credit-Efficient Multi-Host AI Development

*Status*: Proposed

## Date

2026-05-05

## Context

We have multiple machines (NUC `pepper.local`, MacBook M2 `apsnlmac4050.local`) running Ollama, plus access to free cloud AI tiers. Currently we use one host at a time via `.env`. We want to:

1. Use all local hardware simultaneously (distribute work)
2. Supplement with free cloud credits when local models aren't sufficient
3. Track what works (timings, prompt quality) to improve over time
4. Build reusable prompt templates for standard tasks

## Decision

### Architecture: Tiered Routing

```text
┌─────────────────────────────────────────────────────┐
│  Orchestrator (Kiro or llama-cli)                    │
│  Decides: which task → which backend                 │
├─────────────────────────────────────────────────────┤
│  Tier 1: Local Ollama (free, private, unlimited)    │
│    pepper.local — NUC (small models, fast tasks)    │
│    apsnlmac4050.local — M2 (14b models, code)      │
├─────────────────────────────────────────────────────┤
│  Tier 2: Free Cloud APIs (rate-limited, free)       │
│    Google Gemini — 1500 req/day free (AI Studio)    │
│    Groq — fast inference, free tier                  │
│    Cerebras — 24M tokens/day free                   │
│    OpenRouter — :free model variants                 │
├─────────────────────────────────────────────────────┤
│  Tier 3: Paid (Kiro credits, use sparingly)         │
│    Complex architecture, multi-file refactors       │
│    Only when Tier 1+2 can't handle it               │
└─────────────────────────────────────────────────────┘
```

### Free Cloud API Tiers (as of 2026)

| Provider | Free Tier | Models | Best For | API Style |
|----------|-----------|--------|----------|-----------|
| **Google Gemini** | 1500 req/day, no CC needed | Flash, Pro | Design, multimodal | OpenAI-compatible |
| **Groq** | Rate-limited free | Llama, Mistral | Fast inference | OpenAI-compatible |
| **Cerebras** | 24M tokens/day | Llama 3.3 70B | Heavy reasoning | OpenAI-compatible |
| **Mistral** | Experiment tier | Small, Medium | European, cheap | OpenAI-compatible |
| **OpenRouter** | 50 req/day (:free models) | Various | Model variety | OpenAI-compatible |
| **Together AI** | $5 free credits | Open-source catalog | Exploration | OpenAI-compatible |

Key insight: **all use OpenAI-compatible API format**. One provider abstraction covers them all.

### Multi-Host Implementation Plan

**Phase 1: Multi-host config (use existing scanner)**

```env
# .env
OLLAMA_HOSTS=apsnlmac4050.local:11434,pepper.local:11434
OLLAMA_STRATEGY=model-match   # route by model availability
```

The scanner (`src/net/scan.cpp`) already discovers hosts. Wire it into startup:

- Query `/api/tags` on each host → build model→host map
- Route requests to the host that has the requested model
- Fallback: first available host

**Phase 2: Free cloud as fallback provider**

Since all free APIs are OpenAI-compatible, implement one `OpenAIProvider`:

```cpp
// Single provider handles: Groq, Cerebras, Gemini, OpenRouter, Mistral
class OpenAIProvider : public LLMProvider {
  // endpoint + api_key from config
  // POST /v1/chat/completions
};
```

Config:

```env
FALLBACK_PROVIDER=groq
GROQ_API_KEY=gsk_...
GROQ_ENDPOINT=https://api.groq.com/openai/v1
GROQ_MODEL=llama-3.3-70b-versatile
```

**Phase 3: Prompt templates + timing feedback**

The event log already captures `duration_ms` and `tokens_prompt/completion`. Add:

1. **Template storage** — `~/.llama-cli/templates/` with YAML files:

   ```yaml
   # code-review.yml
   name: code-review
   system: "Review this code. Focus on bugs, not style."
   temperature: 0.3
   model_hint: code  # prefers code-specialized model
   ```

2. **Timing feedback** — after each response, log which template + model + host was used. Over time, `make learn` can analyze:
   - Which model/host combo is fastest for which task type
   - Which prompts get good results in 1 shot vs needing retries

3. **Template command** — `/template code-review` switches system prompt + settings

### Orchestration Strategy

**Kiro as orchestrator** makes sense for complex multi-step tasks (it has context about the full project). **llama-cli as worker** handles individual subtasks:

```bash
# Kiro delegates to local llama-cli for mechanical work:
./llama-cli --host pepper.local "explain this function"
./llama-cli --host apsnlmac4050.local "write unit test for X"

# For tasks beyond local model capability:
./llama-cli --provider groq "architect a solution for Y"
```

### Cost Tracking via Event Log

The log already has `tokens_prompt` + `tokens_completion`. Add a cost layer:

```bash
make log --cost   # shows estimated cost per session
```

Cost calculation:

- Local Ollama: $0 (just electricity)
- Groq free: $0 (within limits)
- Kiro: tokens × model_multiplier × base_rate

## Implementation Priority

| Step | Effort | Impact |
|------|--------|--------|
| 1. `OLLAMA_HOSTS` config + model-match routing | Medium | High — use both machines |
| 2. OpenAI-compatible provider (covers all free APIs) | Medium | High — free cloud fallback |
| 3. `/template` command + YAML storage | Small | Medium — consistent prompts |
| 4. `make log --cost` | Small | Low — awareness |
| 5. Auto-routing (complexity → tier selection) | Large | Future — needs ADR-062 |

## Consequences

- Both local machines contribute compute simultaneously
- Free cloud APIs extend capability beyond local model limits
- Event log tracks what works → prompts improve over time
- Templates make standard tasks reproducible and cheap
- Kiro credits reserved for truly complex work only

## References

- @see docs/backlog/006-distributed-ollama.md — original idea
- @see docs/adr/adr-020-provider-abstraction.md — provider interface
- @see docs/adr/adr-027-event-logging.md — timing/token logging
- @see docs/backlog/021-prompt-templates.md — template command
- @see docs/credits-in-ai.md — credit optimization guide
- @see src/net/scan.cpp — existing host scanner
