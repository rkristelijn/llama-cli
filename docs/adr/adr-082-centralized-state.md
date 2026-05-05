# ADR-082: Centralized Application State

*Status*: Proposed · *Date*: 2026-05-05

## Context

State is currently scattered across multiple locations:

- `Config::instance()` — singleton, mutated at runtime by /provider, /model
- `ReplState` — aggregate struct with 20+ fields, passed by reference
- `ModelRegistry` — separate struct, passed as pointer
- `tui::active_theme()` — another singleton
- Local variables in main.cpp (provider unique_ptr)

This makes it hard to reason about "what is the current state?" and creates coupling between modules that shouldn't know about each other.

## Decision

Introduce a single `AppState` struct that owns all mutable application state. Not Redux (immutable + reducers is overkill for single-threaded CLI), but a centralized mutable state with clear ownership.

### Proposed Structure

```cpp
struct AppState {
  // --- Identity ---
  std::string nick = "user";

  // --- Provider/Model ---
  std::string provider = "ollama";    // active provider name
  std::string host = "localhost";     // active host
  std::string port = "11434";         // active port
  std::string model = "auto";        // active model
  ModelRegistry registry;             // all discovered models

  // --- Conversation ---
  std::vector<Message> history;
  bool auto_route = false;

  // --- Display ---
  Theme theme = theme_dark();
  bool color = true;
  bool markdown = true;

  // --- Session ---
  int prompt_count = 0;
  int total_tokens_prompt = 0;
  int total_tokens_completion = 0;
};
```

### Access Pattern

```cpp
// Instead of:
Config::instance().model = "qwen3:30b";
Config::instance().host = "apsnlmac4050";

// Use:
app.model = "qwen3:30b";
app.host = "apsnlmac4050";
```

### Migration Strategy

Phase 1: Create AppState, populate from Config at startup. Keep Config for loading only.
Phase 2: Replace `Config::instance()` mutations with AppState mutations.
Phase 3: Remove Config singleton, make Config a pure loader that returns AppState.

### Why Not Redux/Event Sourcing?

| Approach | Pros | Cons | Verdict |
|----------|------|------|---------|
| Redux (immutable + actions) | Predictable, time-travel debug | Copy overhead, verbose, overkill for CLI | No |
| Event sourcing | Full audit trail | Already have JSONL logger for that | No |
| Observer pattern | Decoupled updates | Complexity for 1 UI thread | Maybe later |
| **Centralized mutable** | Simple, fast, debuggable | No automatic change detection | **Yes** |

### Future: Subscriptions (if needed)

If we ever need reactive updates (e.g., TUI redraws on state change):

```cpp
// Simple callback-based subscription
using StateCallback = std::function<void(const AppState&)>;
std::vector<StateCallback> subscribers;

void update_model(AppState& app, const std::string& model) {
  app.model = model;
  for (auto& cb : subscribers) cb(app);
}
```

But for a CLI tool with sequential input, this is YAGNI. Direct mutation is fine.

## Consequences

- Single place to inspect all runtime state
- Easier to serialize (save/load full session state)
- Easier to test (inject an AppState, verify mutations)
- Removes Config singleton (eventually)
- Breaking change to internal APIs (Phase 2-3)
- No performance impact (single-threaded, no copies)
