---
summary: Hook-Based Plugin Architecture
status: proposed
---

# ADR-111: Hook-Based Plugin Architecture

## Context

llama-cli has agents (personas with permissions and prompts) but no way to customize the pipeline. Users want to:

- Plug in custom markdown renderers or code highlighters
- Block PII before sending to LLM (pre-send hook)
- Transform responses before display (post-receive hook)
- Add custom validation, logging, or enrichment at any stage

Other tools (Claude Code, Amazon Q, Kiro CLI) solve this with hook-based agent configs that allow pre/post processing at defined lifecycle points.

Currently, profiles/agents only control: model, temperature, prompt, and tool permissions. There's no way to intercept or transform the data flowing through the pipeline.

## Decision

### Lifecycle Hooks

Define hooks at each stage of the message pipeline:

```text
User Input → [pre-send] → LLM API → [post-receive] → [pre-render] → Terminal
                                                                        ↓
                                                              [post-render]
```

| Hook | Trigger | Use Cases |
|------|---------|-----------|
| `pre-send` | Before prompt is sent to LLM | PII blocking, context injection, prompt rewriting |
| `post-receive` | After LLM response, before annotation parsing | Response filtering, logging, quality checks |
| `pre-render` | Before markdown/code rendering | Custom renderer swap, syntax highlighting |
| `post-render` | After rendering, before terminal output | Terminal post-processing, accessibility |
| `pre-exec` | Before command execution | Sandboxing, audit logging, command rewriting |
| `post-exec` | After command execution | Output filtering, PII scrubbing from results |

### Hook Definition (in agent config)

```yaml
# ~/.llama-cli/agents.yml or .kiro/agents/myagent.json
agents:
  - name: secure
    model: gemma4:26b
    hooks:
      pre-send:
        - script: scripts/hooks/block-pii.sh    # exit 1 = block
        - script: scripts/hooks/inject-context.sh
      post-receive:
        - script: scripts/hooks/log-response.sh
      pre-render:
        - command: bat --language=cpp --style=plain  # custom highlighter
      pre-exec:
        - script: scripts/hooks/sandbox-check.sh
```

### Hook Contract

Each hook is a script or command that:

- Receives the content on **stdin**
- Outputs transformed content on **stdout**
- Exit code 0 = continue, exit code 1 = block/abort
- Environment variables provide context: `$LLAMA_MODEL`, `$LLAMA_HOST`, `$LLAMA_HOOK_STAGE`

### Built-in Hooks (shipped with llama-cli)

| Hook | Purpose |
|------|---------|
| `block-pii.sh` | Scans for patterns from `.config/.pii`, blocks if found |
| `log-response.sh` | Appends response metadata to events.jsonl |
| `inject-memory.sh` | Adds memory/preferences to context |

### Agent = Profile + Hooks + Permissions

Agents replace the current split between "personas" (agent.h) and "agent configs" (agent_config.h):

```yaml
agents:
  - name: dev
    description: "Development agent with full tool access"
    model: gemma4:26b
    prompt: res/agents/prompts/dev.txt
    temperature: 0.7
    permissions:
      read: allow
      write: ask
      exec: ask
      str_replace: ask
    hooks:
      pre-send:
        - script: scripts/hooks/block-pii.sh
      pre-render:
        - builtin: markdown    # default renderer
        - builtin: highlight   # syntax highlighting

  - name: safe
    description: "Read-only agent, no exec, PII blocked"
    model: llama3.2:3b
    permissions:
      read: allow
      write: deny
      exec: deny
    hooks:
      pre-send:
        - script: scripts/hooks/block-pii.sh
      post-receive:
        - script: scripts/hooks/strip-tool-tags.sh
```

## Implementation Plan

### Phase 1: Hook infrastructure

- `HookRunner` class: loads hook config, executes scripts with stdin/stdout piping
- Integrate at the 4 main pipeline points in `repl_chat.cpp`
- `pre-send` and `post-receive` first (highest value)

### Phase 2: Merge agents

- Unify `agent.h` (personas) and `agent_config.h` into single `Agent` struct
- Load from `.kiro/agents/*.json` or `~/.llama-cli/agents.yml`
- `/agent` command switches all: model, prompt, permissions, hooks

### Phase 3: Built-in hooks

- Ship `block-pii.sh` (uses `.config/.pii` patterns)
- Ship `inject-memory.sh` (loads `.cache/llama-memory.md`)
- Document hook API for custom plugins

## Consequences

- **Extensible**: any script can be a hook — language agnostic
- **Composable**: multiple hooks chain (stdout of one → stdin of next)
- **Safe**: pre-send PII blocking prevents accidental data leaks to LLM
- **Backward compatible**: agents without hooks work exactly as before
- **Debuggable**: `TRACE=1` shows hook execution and timing
- **No runtime dependencies**: hooks are plain scripts, no plugin framework needed
