# ADR-089: Hierarchical Host → Provider → Model Selection

*Status*: Proposed · *Date*: 2026-05-06

## Context

The current model/provider switching has several problems:

1. **`/nick` is display-only** — sets `Config::instance().nick` but has no `@host:provider:model` syntax for quick switching.
2. **Duplicate registry entries** — `amazon-q` (via `which q`) and `kiro-cli` (via `which kiro-cli-chat`) both register separately, but they're the same backend. The `amazon-q` entry gets only 1 hardcoded model while `kiro-cli` gets the full list.
3. **No `/host` command** — users can't select a host first and then drill down to models on that host.
4. **`/provider` switch is incomplete** — `switch_provider` creates a new provider instance but doesn't consistently update `Config::instance().host` or `.model`, leading to stale state.
5. **No quick-switch syntax** — switching requires multiple interactive steps instead of a one-liner.

## Decision

### 1. Hierarchical Selection Model

Selection follows a strict hierarchy: **Host → Provider → Model**.

```text
Host (where it runs)
├── cloud                    → kiro-cli, gemini, tgpt
├── <hostname>:11434       → ollama
├── jarvis:11434             → ollama
└── localhost:11434          → ollama

Provider (how to talk to it)
├── ollama                   → HTTP API to Ollama server
├── kiro-cli                 → kiro-cli-chat subprocess
├── gemini                   → gemini CLI subprocess
└── tgpt                     → tgpt subprocess

Model (what to run)
├── qwen2.5-coder:14b       → on ollama@<hostname>
├── claude-sonnet-4-20250514    → on kiro-cli@cloud
└── gemini-2.5-pro           → on gemini@cloud
```

### 2. New Commands

| Command | Behavior |
|---------|----------|
| `/host` | List unique hosts, select by number. Filters `/model` to that host. |
| `/host <name>` | Switch directly to a host (hostname or IP:port or "cloud"). |
| `/provider` | List providers (unchanged UX but fixes state update). |
| `/provider <name>` | Switch provider AND update host/model to first available. |
| `/model` | List models (filtered by current host if set, or all). |
| `/model <name>` | Switch to model by name (auto-resolves host+provider). |

### 3. Quick-Switch Syntax via `/use`

A single command to set any combination:

```text
/use model                       → switch model (keep host+provider)
/use provider:model              → switch provider + model
/use @host                       → switch host (pick first provider+model)
/use @host:provider:model        → switch everything
```

Examples:

```text
/use qwen2.5-coder:14b                    → model only
/use ollama:gemma4:26b                     → provider + model
/use @<hostname>:11434                   → host only
/use @cloud:kiro-cli:claude-sonnet-4-20250514  → full path
```

### 4. `/nick` Stays Display-Only

`/nick` keeps its current behavior (set display name). The quick-switch syntax moves to `/use` to avoid overloading `/nick` with unrelated semantics.

### 5. Fix Registry Deduplication

Remove the separate `amazon-q` entry from `registry.cpp`. The `kiro-cli` section already discovers all models via `kiro-cli-chat --list-models`. The `which q` check is redundant since `kiro-cli-chat` is the actual binary used.

**Before:**

```cpp
// --- Amazon Q Developer (via q CLI) ---
ExecResult q_check = cmd_exec("which q", 3, 200);
if (q_check.exit_code == 0) { ... }  // hardcoded single entry

// --- kiro-cli ---
ExecResult kiro_check = cmd_exec("which kiro-cli-chat", 3, 200);
if (kiro_check.exit_code == 0) { ... }  // full model list
```

**After:**

```cpp
// --- kiro-cli (Amazon Q Developer with model selection) ---
// Removed separate "which q" check — kiro-cli-chat covers all amazon-q models.
ExecResult kiro_check = cmd_exec("which kiro-cli-chat", 3, 200);
if (kiro_check.exit_code == 0) { ... }  // full model list, provider="kiro-cli"
```

### 6. Fix `/provider` State Consistency

When switching provider, update ALL related config fields atomically:

```cpp
// In switch_provider callback:
Config::instance().provider = new_provider;
// Pick first available model on this provider from registry
if (registry) {
  for (const auto& m : registry->models) {
    if (m.provider == new_provider && m.available) {
      Config::instance().model = m.name;
      if (m.host != "cloud") {
        auto colon = m.host.find(':');
        Config::instance().host = m.host.substr(0, colon);
        Config::instance().port = m.host.substr(colon + 1);
      }
      break;
    }
  }
}
// Recreate provider instance
provider = create_provider(Config::instance());
```

### 7. `/host` Command (New)

Lists unique hosts from the registry with model counts:

```text
/host
  1. <hostname>:11434  (ollama, 5 models)
  2. jarvis:11434        (ollama, 2 models)
  3. localhost:11434     (ollama, 1 model)
  4. cloud              (kiro-cli: 8, gemini: 3, tgpt: 1)
  Select [1-4]: _
```

After selecting a host, `/model` shows only models on that host.

### 8. Implementation Order

1. Remove `amazon-q` duplicate from `registry.cpp`
2. Add `/host` command to `repl_commands.cpp`
3. Fix `/provider` to update config atomically + recreate provider
4. Add `/use` quick-switch command
5. Update `/model` to respect host filter

## Consequences

- **Cleaner registry** — no duplicate entries for the same backend
- **Consistent state** — provider switch always updates host+model+provider together
- **Hierarchical navigation** — `/host` → `/provider` → `/model` for exploration
- **Quick switching** — `/use @host:provider:model` for power users
- **No breaking changes to `/nick`** — display name stays separate from routing
- **Backward compatible** — `/model` and `/provider` keep working as before, just fixed

## Future Work (Next Session)

Features planned for follow-up implementation:

| Feature | Description | Status |
|---------|-------------|--------|
| Multi-host load balancing | Round-robin across hosts with same model, not just failover | ⬚ Planned |
| `/delegate` command | Send a task to a specific model/host while staying in current session | ⬚ Planned |
| `/usage` enhanced | Real token counts, cost tracking per provider, session totals | ⬚ Planned |
| `/stats` command | Tokens/sec per model (rolling average), quality ratings, response times | ⬚ Planned |
| Keystroke counter | Track keystrokes per session for productivity metrics | ⬚ Planned |
| Session focus | Named sessions with time tracking (focus timer, Pomodoro-style) | ⬚ Planned |
| Parallel delegation | Split complex tasks across multiple models simultaneously | ⬚ Planned |
| Cost budget | Warn/block when session cost exceeds configured limit | ⬚ Planned |
