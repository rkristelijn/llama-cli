# ADR-096: Multi-Agent Implementation Plan

*Status*: Superseded · *Date*: 2026-05-06 · *Implements*: [ADR-084](adr-084-planner-executor.md), [ADR-085](adr-085-multi-agent.md)

> **Superseded (2026-05-07)**: The orchestrator/planner/subagent architecture was replaced
> by a simpler front-LLM async delegation model. The LLM proposes `<delegate>` annotations,
> user approves, tasks run in background via `std::async`. See ADR-099.

## Context

ADR-084 and ADR-085 describe the vision for planner/executor separation and multi-agent support. The current codebase has:

- `LLMProvider` interface (ADR-020) — abstract chat/stream/list
- `planner.cpp` — prompt complexity classifier + host/model routing
- `agent.cpp` — persona loading (system prompt overlays)
- Provider registry with multi-host support
- REPL with injectable `ChatFn` / `StreamChatFn`

We need to evolve from "single LLM loop" to "orchestrated multi-agent" without breaking existing functionality. This ADR defines the incremental implementation phases.

## Architecture (inspired by OpenCode)

OpenCode's agent model distinguishes between:

- **Primary agents** — user-facing, switchable via Tab (build, plan)
- **Subagents** — invoked by primary agents or via `@mention` (general, explore)
- **Permission system** — per-agent tool access (allow/ask/deny per tool)
- **Task tool** — primary agents spawn subagents as child sessions

We adapt this to our C++ codebase and local-first constraints:

```text
┌─────────────────────────────────────────────────────┐
│                    Orchestrator                       │
│  (receives user request, decides routing)            │
├─────────────────────────────────────────────────────┤
│                                                      │
│  Primary Agents (user switches via /agent)           │
│  ┌──────────┐   ┌──────────┐                        │
│  │  build   │   │   plan   │                        │
│  │ (all     │   │ (read-   │                        │
│  │  tools)  │   │  only)   │                        │
│  └──────────┘   └──────────┘                        │
│                                                      │
│  Subagents (invoked by primary or via @mention)      │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐        │
│  │ general  │   │ explore  │   │ reviewer  │        │
│  │(multi-   │   │(read-    │   │(validate  │        │
│  │ step)    │   │ only)    │   │ output)   │        │
│  └──────────┘   └──────────┘   └──────────┘        │
│       │              │               │               │
│       ▼              ▼               ▼               │
│  ┌─────────────────────────────────────────┐        │
│  │         Provider Registry                │        │
│  │  (ollama hosts, gemini, tgpt, kiro)     │        │
│  └─────────────────────────────────────────┘        │
│                                                      │
└─────────────────────────────────────────────────────┘
```

### Key Concepts (from OpenCode)

| Concept | OpenCode | Our adaptation |
|---------|----------|----------------|
| Agent types | primary / subagent | Same: primary (build, plan) + subagents |
| Permissions | allow/ask/deny per tool | Same: per-agent tool access control |
| Subagent invocation | `@mention` or automatic | `@mention` in prompt or orchestrator decides |
| Child sessions | Subagent runs in child session | Subagent has own message history, results fed back |
| Model per agent | Configurable per agent | Route to different host/model per agent role |
| Max steps | Configurable limit | Bounded loop with retry limit |

### Agent Definitions

| Agent | Mode | Tools | Model preference | Purpose |
|-------|------|-------|-----------------|---------|
| **build** | primary | all | Current config | Default — full development |
| **plan** | primary | read-only | Large (27B+) | Analysis, planning, no edits |
| **general** | subagent | all except todo | Fast (3-7B) | Multi-step task execution |
| **explore** | subagent | read, grep, glob | Fast (3B) | Codebase exploration |
| **reviewer** | subagent | read-only | Capable (14B+) | Validate output |

### Task Schema (from ADR-084)

```json
{
  "goal": "Fix the bug in parser.cpp",
  "steps": [
    { "id": 1, "tool": "read", "input": { "path": "src/parser.cpp" } },
    { "id": 2, "tool": "str_replace", "input": { "path": "src/parser.cpp", "old": "...", "new": "..." } },
    { "id": 3, "tool": "exec", "input": { "cmd": "make test-unit" } }
  ]
}
```

### Prompt Templates per Agent

Each agent needs a focused system prompt that tells the LLM exactly what role it plays and what output format is expected. Without this, delegation fails — the subagent doesn't know its boundaries.

Templates live in `res/agents/prompts/`:

| File | Agent | Key instruction |
|------|-------|-----------------|
| `build.txt` | build | Current system-prompt.txt (full tool access) |
| `plan.txt` | plan | "Analyze only. Output a JSON task plan. Do not execute." |
| `general.txt` | general | "Execute the given step. Use tools. Be precise and minimal." |
| `explore.txt` | explore | "Search and read only. Report findings. No modifications." |
| `reviewer.txt` | reviewer | "Validate the output. Reply PASS, RETRY with reason, or FAIL." |

Template format supports variables:

```text
You are the {{agent_name}} agent for llama-cli.
Your role: {{description}}

{{#if goal}}Current task: {{goal}}{{/if}}
{{#if step}}Current step: {{step.action}} ({{step.id}}/{{total_steps}}){{/if}}

{{base_instructions}}
```

This ensures:

- The planner knows to output JSON, not prose
- The executor knows which step it's working on
- The reviewer has clear pass/fail criteria
- Context is minimal (only what the agent needs, not full history)

### Telemetry & Model Performance Metrics

Every LLM call is already logged via `LOG_EVENT` (ADR-027). We extend this with per-model aggregated metrics stored in `~/.llama-cli/metrics.json`:

```json
{
  "models": {
    "gemma4:e4b": {
      "calls": 142,
      "total_tokens": 89420,
      "total_duration_ms": 312000,
      "avg_tokens_per_sec": 28.6,
      "success_rate": 0.94,
      "last_used": "2026-05-06T14:30:00Z",
      "by_agent": {
        "build": { "calls": 120, "avg_tokens_per_sec": 29.1 },
        "explore": { "calls": 22, "avg_tokens_per_sec": 25.3 }
      }
    },
    "qwen3:14b": {
      "calls": 38,
      "total_tokens": 52100,
      "total_duration_ms": 198000,
      "avg_tokens_per_sec": 12.4,
      "success_rate": 0.97,
      "last_used": "2026-05-06T14:25:00Z",
      "by_agent": {
        "plan": { "calls": 30, "avg_tokens_per_sec": 12.8 },
        "reviewer": { "calls": 8, "avg_tokens_per_sec": 11.2 }
      }
    }
  }
}
```

This enables:

- **Smart routing**: pick the fastest model that's good enough for the task
- **Quality feedback**: if a model's success_rate drops, route away from it
- **Cost awareness**: track token usage per agent role
- **`/usage` command**: show real performance data instead of static estimates

Metrics are updated after every LLM call (append-only, periodic flush). The orchestrator reads metrics to inform routing decisions.

### Routing Rule Engine (data-driven, no LLM needed)

The orchestrator uses a simple rule engine to pick the best host/model/provider per agent role. Rules are evaluated against the metrics file — no LLM call needed for routing.

```text
┌─────────────────────────────────────────────────────┐
│              Routing Rule Engine                      │
│                                                      │
│  Input: agent_role, prompt_complexity, metrics.json  │
│  Output: { host, model, provider }                   │
│                                                      │
│  Rules (evaluated in order, first match wins):       │
│                                                      │
│  1. If agent has explicit model config → use it      │
│  2. If metrics show model success_rate < 0.7         │
│     for this agent_role → skip it                    │
│  3. Pick model with best score for role:             │
│     score = tokens_per_sec × success_rate            │
│            × recency_bonus                           │
│  4. If no metrics yet → use complexity heuristic     │
│     (existing classify_prompt → size range)          │
│  5. Fallback → current config (user's default)       │
│                                                      │
└─────────────────────────────────────────────────────┘
```

**Score formula:**

```text
score(model, role) = avg_tokens_per_sec[role]
                   × success_rate[role]
                   × recency_factor(last_used)
```

Where:

- `recency_factor` = 1.0 if used in last hour, decays to 0.5 after 24h (prefer warm models)
- `success_rate` = completed_calls / total_calls per agent role
- A call is "successful" if it didn't timeout, return empty, or produce unparseable output

**Bootstrap (cold start):**

- No metrics yet → fall back to existing `classify_prompt` heuristic (model size by complexity)
- After ~10 calls per model, the rule engine has enough data to route intelligently
- First few calls deliberately spread across available models to gather baseline data

**Prompt effectiveness tracking:**

- Each prompt template gets a `template_id` in the metrics
- Track: did the LLM follow the format? (e.g., planner returned valid JSON? reviewer said PASS/RETRY/FAIL?)
- If a prompt's format_compliance drops below 80%, flag it for revision
- This lets us iterate on prompts without guessing

```json
{
  "prompts": {
    "plan.txt": {
      "calls": 30,
      "format_compliant": 26,
      "compliance_rate": 0.87,
      "avg_retries_needed": 0.4
    },
    "reviewer.txt": {
      "calls": 15,
      "format_compliant": 14,
      "compliance_rate": 0.93,
      "avg_retries_needed": 0.1
    }
  }
}
```

This means: **from day 1 we collect data**, and the system gets smarter over time without needing a meta-LLM to decide routing.

### Orchestration Flow

```text
1. User sends prompt
2. If @mention → route to named subagent
3. If /plan active → use plan agent (read-only)
4. If /auto enabled → classify complexity:
   a. Simple → direct single LLM (current behavior)
   b. Complex → plan agent decomposes, general subagent executes steps
5. Subagent runs in child context (own message history)
6. Each tool use requires permission check (allow/ask/deny)
7. Results fed back to parent agent
8. Optional: reviewer subagent validates before presenting to user
```

## Implementation Phases

### Phase 1: Task Schema + Orchestrator Skeleton

**Goal**: Define data structures, agent config, and orchestrator interface. No behavior change.

Files:

- `src/orchestrator/task.h` — TaskPlan, TaskStep, StepResult structs
- `src/orchestrator/orchestrator.h/.cpp` — Skeleton (always pass-through)
- Unit tests for task parsing/serialization

**Acceptance criteria**:

- `make build` passes
- `make test-unit` passes
- Existing REPL behavior unchanged

### Phase 2: Prompt Templates + Telemetrie

**Goal**: Agent-specifieke prompts en per-model performance tracking.

Files:

- `res/agents/prompts/build.txt` — Symlink/copy van huidige system-prompt.txt
- `res/agents/prompts/plan.txt` — Planner prompt (JSON output)
- `res/agents/prompts/general.txt` — Executor prompt (step-focused)
- `res/agents/prompts/explore.txt` — Read-only explorer prompt
- `res/agents/prompts/reviewer.txt` — Validator prompt (PASS/RETRY/FAIL)
- `src/orchestrator/prompt_template.h/.cpp` — Template rendering (variable substitution)
- `src/orchestrator/metrics.h/.cpp` — Per-model metrics (read/write JSON)
- Unit tests for template rendering + metrics update

**Acceptance criteria**:

- Templates render with variables (agent_name, goal, step)
- Metrics file created/updated after LLM calls
- `/usage` shows real tokens/sec per model
- `make test-unit` passes

### Phase 3: Agent Config + Permission System

**Goal**: Define agents with per-agent permissions. Extend existing persona system.

Files:

- `src/orchestrator/agent_config.h` — AgentConfig struct (name, mode, permissions, model, prompt)
- `res/agents/agents.yml` — Agent definitions (build, plan, general, explore, reviewer)
- Permission check integrated into existing annotation handlers

**Acceptance criteria**:

- `/agent plan` switches to read-only mode (edits blocked)
- `/agent build` restores full access
- Permissions enforced on write/exec annotations

### Phase 4: Subagent Invocation via @mention

**Goal**: `@general do X` spawns a subagent with its own context.

Files:

- `src/orchestrator/subagent.h/.cpp` — Spawn subagent, manage child context
- Parse `@agentname` from user input
- Subagent uses its own system prompt + model routing

**Acceptance criteria**:

- `@explore find all TODO comments` runs explore subagent (read-only)
- Subagent result displayed inline in parent conversation
- Subagent respects its permission set

### Phase 5: Plan Agent + Task Decomposition

**Goal**: Plan agent decomposes complex requests into structured task plans.

Files:

- `src/orchestrator/planner_agent.cpp` — Sends to large model, parses JSON plan
- Plan displayed to user for approval before execution

**Acceptance criteria**:

- `/plan fix the bug in parser.cpp` generates a task plan
- User can approve/reject/edit the plan
- Invalid JSON from LLM → graceful fallback to single-shot

### Phase 6: General Subagent + Execution Loop

**Goal**: Approved plan steps executed by general subagent with user confirmation per step.

Files:

- `src/orchestrator/executor_loop.cpp` — Iterate plan steps, invoke general subagent
- Reuses existing annotation handlers (write/str_replace/exec/read)
- Optional reviewer subagent validates after each step

**Acceptance criteria**:

- Steps execute sequentially with user confirmation
- Failed steps can retry (max 2) or skip
- Progress display: "Step 2/5: writing parser.cpp..."
- Metrics updated per step (which model, tokens/sec, success/fail)

## Design Decisions

| Decision | Rationale | OpenCode parallel |
|----------|-----------|-------------------|
| Primary + subagent split | Clear UX: Tab switches primary, @mention invokes subagent | Same model |
| Permission system (allow/ask/deny) | Safety — granular tool access per agent | Same model |
| JSON task schema | Parseable, loggable, resumable | OpenCode uses structured tool calls |
| User confirmation per step | Safety — no autonomous execution (ADR-019) | OpenCode uses "ask" permission |
| Fallback to single-shot on parse failure | Graceful degradation | Same approach |
| Reuse existing annotation handlers | No duplicate file/exec logic | OpenCode reuses tool implementations |
| Orchestrator is not an LLM | Deterministic control flow | Same — orchestrator is code, not AI |
| Simple prompts bypass orchestrator | No overhead for trivial queries | Same — only complex tasks get decomposed |
| Agent config in YAML | Easy to customize, no recompile | OpenCode uses JSON + markdown |
| Child context per subagent | Isolation, focused conversation | OpenCode uses child sessions |

## Consequences

### Positive

- Better results for complex tasks through decomposition
- Each phase is independently shippable and testable
- Existing UX preserved for simple interactions
- Cost-efficient: cheap models for simple steps

### Negative

- Latency increase for complex tasks (multiple LLM calls)
- JSON parsing from LLMs is unreliable (needs robust fallback)
- More code to maintain

### Future Considerations (not in scope)

- **Rate limiting** — throttle calls per host to avoid overloading local Ollama instances
- **Load balancing** — distribute calls across hosts based on current queue depth / GPU utilization
- These build naturally on the metrics + routing engine once multi-host is actively used

## References

- [ADR-020: Provider Abstraction](adr-020-provider-abstraction.md)
- [ADR-079: Auto-Routing](adr-079-auto-routing.md)
- [ADR-084: Planner/Executor](adr-084-planner-executor.md)
- [ADR-085: Multi-Agent](adr-085-multi-agent.md)
