# ADR-051: Kiro Agent Configuration for llama-cli Development

*Status*: Implemented · *Date*: 2026-04-19

## Context

Development on llama-cli uses Kiro CLI as the AI-assisted development tool. Without project-specific configuration, Kiro uses its default agent which lacks knowledge of:

- The project's `make` targets and build conventions
- CONTRIBUTING.md code style rules
- The ability to delegate small tasks to llama-cli itself
- Which Ollama model to use for what type of task

This leads to repeated corrections ("use `make build` not `cmake`", "follow the shell script conventions", etc.).

## Decision

Configure Kiro agents in `.kiro/agents/` to encode project knowledge. Start with a single general-purpose agent; split into specialized agents only when needed.

### Current: Single Agent (`llama-dev`)

One agent that covers all development tasks:

| Capability | How |
|-----------|-----|
| Build & quality | Uses `make` targets exclusively; `make help` loaded at spawn |
| Code conventions | CONTRIBUTING.md loaded as resource |
| Task awareness | TODO.md loaded as resource |
| Local AI delegation | Delegates to `./build/llama-cli` for small research tasks |
| Model selection | qwen2.5-coder:14b for code, gemma4:26b for general |

### When to Split

Split into multiple agents when:

1. **System prompts conflict** — e.g. a review agent needs "be critical" while a coding agent needs "just implement it"
2. **Tool permissions differ** — e.g. a docs agent shouldn't need `execute_bash`
3. **Subagent pipelines** — e.g. research → implement → review, each with a specialized agent

### Potential Future Agents

| Agent | When to create | System prompt focus |
|-------|---------------|-------------------|
| `llama-review` | When PR review becomes a regular workflow | Critical analysis, find bugs, suggest improvements |
| `llama-code` | When code-only tasks need a different model/prompt | Minimal explanation, code output only |
| `llama-docs` | When documentation tasks are frequent | ADR format, backlog format, user-guide style |

### Agent Configuration

Agents live in `.kiro/agents/*.json` and are committed to the repo so all contributors benefit. Key fields:

```json
{
  "name": "llama-dev",
  "prompt": "...",           // Project-specific system prompt
  "tools": ["@builtin"],    // Available tools
  "resources": [             // Auto-loaded context
    "file://CONTRIBUTING.md",
    "file://Makefile",
    "file://TODO.md"
  ],
  "hooks": {
    "agentSpawn": [{         // Run at agent start
      "command": "make help 2>/dev/null | head -40"
    }]
  }
}
```

### llama-cli as a Development Tool

The agent can delegate to llama-cli for tasks that don't need Kiro's full context:

```bash
# Code question — use the coding model
./build/llama-cli --model qwen2.5-coder:14b \
  --system-prompt="Be concise. Code only." \
  "show the Post signature with ContentReceiver"

# File analysis — pipe content as context
cat src/ollama/ollama.cpp | ./build/llama-cli --model qwen2.5-coder:14b \
  "what does this file do? 3 bullet points"

# General question — use the default model
./build/llama-cli "what is the difference between streaming and buffered HTTP?"
```

This saves tokens on the Kiro side and uses local compute for simple lookups.

## Consequences

### Positive

- New contributors get project conventions automatically
- `make` targets are always used correctly
- llama-cli is dogfooded during its own development
- Agent config is version-controlled and reviewable

### Negative

- `.kiro/` directory added to repo (small overhead)
- Agent config must be maintained as project evolves
- llama-cli delegation only works when Ollama is running

## References

- [CONTRIBUTING.md](../../CONTRIBUTING.md) — Code conventions
- [ADR-044](adr-044-tidy-boilerplate.md) — Makefile structure
- [ADR-050](adr-050-reality-check-roadmap.md) — Project roadmap and positioning
- `.kiro/agents/llama-dev.json` — Current agent configuration
