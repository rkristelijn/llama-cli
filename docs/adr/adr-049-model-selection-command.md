# ADR-049: Interactive Model Selection Command

*Status*: Implemented · *Date*: 2026-04-18 · *Context*: Users need a way to switch models during a REPL session without restarting. The `/model` command provides interactive model selection from the Ollama server.

## Problem

- Users must restart llama-cli to switch models
- No way to discover available models from within the REPL
- Manual model name entry is error-prone

## Options Considered

| Option | Pros | Cons |
|--------|------|------|
| Require restart with `--model` flag | Simple | Poor UX, loses conversation history |
| `/model <name>` direct input | Fast for known models | No discovery, typos fail silently |
| Interactive numbered menu | Discoverable, safe, user-friendly | Requires I/O handling |

## Decision

Implement `/model` command with interactive numbered menu:

1. Fetch available models from Ollama `/api/tags` endpoint
2. Display numbered list (1-indexed for user friendliness)
3. Prompt user to enter selection number
4. Validate input (range check, numeric validation)
5. Update `Config::instance().model` with selected model
6. Subsequent prompts use the new model

## Implementation Details

### New Functions

**`ollama.cpp`**: `get_available_models(const Config& cfg)`

- Calls `GET /api/tags` on configured Ollama server
- Parses JSON response to extract model names
- Returns `std::vector<std::string>` (empty on error)

**`repl.cpp`**: `handle_model_selection(ReplState& s)`

- Fetches models via `get_available_models()`
- Displays numbered menu
- Reads user input from `s.in`
- Validates selection (1-indexed, in range)
- Updates `Config::instance().model`
- Provides feedback: `[model set to X]`, `[out of range]`, `[invalid input]`

**`repl.cpp`**: Updated `handle_command()`

- Added `else if (input.command == "model")` branch
- Calls `handle_model_selection(s)`

### User Experience

```text
> /model
Available models:
  1. qwen2.5-coder:32b
  2. qwen2.5-coder:7b
  3. llama3.2:3b
Select model (1-3): 2
[model set to qwen2.5-coder:7b]
```

Error cases:

- No models available: `No models available on host:port`
- Invalid input: `[invalid input]`
- Out of range: `[out of range]`
- Cancelled (EOF): `[cancelled]`

## Rationale

- **Numbered menu**: Safer than free-form text entry; prevents typos
- **1-indexed display**: Matches user expectations (not 0-indexed)
- **Config update**: Changes apply immediately to next prompt
- **No restart needed**: Preserves conversation history
- **Error handling**: Graceful fallback if server unreachable

## Consequences

- `ollama.cpp` now has a public `get_available_models()` function
- `repl.cpp` includes new `handle_model_selection()` function
- `/help` text updated to document `/model` command
- Unit tests added for valid/invalid/out-of-range selections
- Model changes persist only for current session (not saved to config file)

## Testing

Unit tests in `repl_test.cpp`:

- Valid selection (1 from available models)
- Out of range selection (999)
- Non-numeric input (abc)

Integration tests verify `/model` works with real Ollama server.
