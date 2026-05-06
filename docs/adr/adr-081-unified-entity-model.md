# ADR-081: Unified Provider-Host-Model Entity Model

*Status*: Proposed · *Date*: 2026-05-05

## Context

After implementing multi-provider (ollama, tgpt, gemini), multi-host, and multi-model support, the UX is inconsistent:

- `/model` shows models without clear host attribution
- `/provider` doesn't show a numbered list like `/model` does
- No visibility into tokens/sec, cost, or capabilities per model
- No prompt gate (preflight) before expensive calls
- kiro-cli not yet available as provider
- `/usage` doesn't track real token counts across providers

## Decision

### 1. Entity Model

```text
Provider (type: ollama | tgpt | gemini | kiro-cli)
 └── Host (address: host:port | "cloud" | "local-cli")
      └── Model (name, metadata)
           ├── tokens_per_sec: float    (from preflight benchmark)
           ├── cost_tier: free | cheap | expensive
           ├── capabilities: [code, vision, general, reasoning, current-info]
           └── status: ready | loading | offline
```

**Relationships:**

- One Provider has 1..N Hosts
- One Host has 1..N Models
- Ollama: multiple hosts, each with multiple models
- tgpt/gemini/kiro-cli: single virtual host ("cloud"/"local-cli"), single model

### 2. Provider Registry

All providers register at startup into a unified registry:

```text
┌─────────────────────────────────────────────────────────────┐
│ Provider Registry                                           │
├──────────┬────────────────────┬─────────────────────────────┤
│ Provider │ Host               │ Models                      │
├──────────┼────────────────────┼─────────────────────────────┤
│ ollama   │ <hostname>:11434 │ qwen2.5-coder:14b (42 t/s) │
│          │                    │ gemma4:26b (8 t/s)          │
│          │                    │ qwen3.6:27b (3 t/s)         │
│ ollama   │ jarvis:11434       │ llama3.2:3b (85 t/s)        │
│ ollama   │ pepper:11434       │ llama3.2:3b (85 t/s)        │
│ ollama   │ localhost:11434    │ gemma2:2b (60 t/s)          │
│ tgpt     │ cloud              │ tgpt-default (? t/s, free)  │
│ gemini   │ cloud              │ gemini-cli (? t/s, free)    │
│ kiro-cli │ cloud              │ kiro (? t/s, expensive)     │
└──────────┴────────────────────┴─────────────────────────────┘
```

### 3. Consistent REPL Commands

All list commands use the same numbered-selection pattern:

```bash
/provider                    # list providers with numbers
  1. ollama (4 hosts, 12 models)
  2. tgpt (cloud, free)
  3. gemini (cloud, free)
  4. kiro-cli (cloud, expensive)
  Select [1-4]: _

/model                       # list ALL models across all providers/hosts
  1. qwen2.5-coder:14b    ollama@<hostname>   42 t/s  code
  2. gemma4:26b            ollama@<hostname>    8 t/s  general
  3. llama3.2:3b           ollama@jarvis          85 t/s  general
  4. llama3.2:3b           ollama@pepper          85 t/s  general
  5. tgpt-default          tgpt@cloud              ? t/s  general
  6. gemini-cli            gemini@cloud            ? t/s  general
  Select [1-6]: _

/usage                       # session stats
  Session: 12 messages, 3 LLM calls
  Tokens: ~2400 prompt, ~1800 completion
  Cost: free (all local)
  Providers used: ollama@<hostname> (2x), ollama@jarvis (1x)
```

### 4. Prompt Gate (Preflight Validation)

Before routing to expensive providers (kiro-cli, or large models), validate:

| Check | Action |
|-------|--------|
| Empty/whitespace prompt | Reject: "Please enter a question" |
| < 3 words | Warn: "Very short prompt — did you mean to ask more?" |
| Repeat of previous prompt | Warn: "Same as last prompt — rephrase?" |
| > 500 words to cloud provider | Warn: "Long prompt to cloud — this may be slow/expensive" |
| Unclear/vague (no verb, no question mark, no code) | Suggest: template (Given/When/Then) |

The gate is advisory (warnings), not blocking. User can always proceed.

### 5. Prompt Template Suggestion

When the gate detects a vague prompt, suggest structured format:

```text
Your prompt seems vague. Try structuring it:

  Given: [context or what you're working on]
  When: [the specific question or task]
  Then: [what format you want the answer in]

Example: Given: I have a REST API in Go
         When: add pagination to the /users endpoint
         Then: show the code changes needed
```

### 6. kiro-cli Provider

```text
Provider: kiro-cli
Binary: kiro (if installed)
Invocation: kiro chat --prompt "<prompt>" (non-interactive)
Cost tier: expensive (uses cloud credits)
Capabilities: [code, reasoning, general]
When to use: complex architecture, multi-file refactoring, when local models fail
```

### 7. Token Tracking (/usage)

Every LLM call logs to events.jsonl:

- `tokens_prompt`: from Ollama API response (exact)
- `tokens_completion`: from Ollama API response (exact)
- For tgpt/gemini/kiro: estimate from response length (chars/4)
- `/usage` aggregates from session history + event log

### 8. Startup Behavior (LLAMA_PROVIDER=auto)

```text
llama-cli
  Scanning providers...
  ✓ ollama: 4 hosts, 12 models (fastest: llama3.2:3b@jarvis 85 t/s)
  ✓ tgpt: ready (cloud, free)
  ✓ gemini: ready (cloud, free)
  ✗ kiro-cli: not installed
  Auto-routing enabled. /auto off to disable.
gius>
```

## Consequences

- Single entity model (Provider→Host→Model) replaces ad-hoc multi-host logic
- All REPL list commands are consistent (numbered selection)
- Prompt gate prevents wasted tokens on bad prompts
- Token tracking enables cost awareness
- kiro-cli as escalation path for complex tasks
- Benchmark data (t/s) enables informed routing decisions
- Breaking change: `/model` and `/provider` output format changes
