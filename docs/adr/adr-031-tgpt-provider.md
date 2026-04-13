# ADR-031: tgpt Provider Integration

*Status*: Proposed · *Date*: 2026-04-13 · *Context*: Expand provider options with a free, CLI-based alternative to Ollama and Gemini.

## Decision

Integrate `tgpt` (Terminal GPT) as an additional provider, following the provider abstraction defined in ADR-020. The integration will invoke the `tgpt` CLI tool in headless mode, similar to the GeminiProvider approach in ADR-021.

### Interaction Pattern

1. **Install Check**: Verify `tgpt` is available in PATH during provider initialization.
2. **History Format**: Collapse conversation history into a single prompt string.
3. **Subprocess Invocation**: Execute `tgpt -p "<formatted_prompt>"` using the existing `exec` module.
4. **Output Capture**: Capture stdout and return to the REPL.

## Rationale

1. **Zero Cost**: Unlike OpenAI API, tgpt uses free providers (Pollinations.AI, FreeGpt, etc.) without requiring an API key.
2. **Provider Diversity**: Adds an alternative when Ollama is unavailable or Gemini API limits are reached.
3. **Code Reuse**: Leverages existing `exec` module (ADR-015) for timeouts and output handling.
4. **Cross-Platform**: tgpt supports Linux, macOS, and Windows.

## Implementation Details

### Installation Detection

```cpp
// ProviderFactory checks for tgpt availability
bool has_tgpt = (exec("which tgpt") == 0);
```

### Execution Strategy

```cpp
// Build command similar to GeminiProvider
std::string cmd = "tgpt -p \"" + escape(collapsed_history) + "\"";
ExecResult res = cmd_exec(cmd, cfg.timeout, cfg.max_output);
```

### File Input Support

tgpt supports piping, enabling the `/read` functionality:

```bash
cat file.txt | tgpt "summarize this"
```

This maps directly to the existing stdin/file input pattern from ADR-030.

## Alternatives Considered

| Alternative | Why Not Chosen |
| :--- | :--- |
| **Direct OpenAI API** | Requires paid API key, contradicts "free local assistant" goal. |
| **Web scraper (Playwright)** | Complex, fragile, violates privacy principle. |
| **Interactive REPL mode** | Hard to control programmatically; output parsing unreliable. |

## Consequences

### Positive

- Free alternative provider for users without Ollama or API keys.
- Simple installation via `brew install tgpt` or `curl | bash`.
- Supports image generation via `--img` flag (future extensibility).

### Negative

- **Reliability**: Free providers may have rate limits or downtime.
- **Latency**: Process spawn per request (same as GeminiProvider).
- **Security**: Shell escaping critical to prevent injection.
- **Context Window**: Free providers may have smaller context limits.

## Security Considerations

The `-s` (shell mode) flag must never be enabled automatically. Only text mode (`-p`) is used:

```cpp
// SAFE: text-only mode
std::string cmd = "tgpt -p \"" + escaped_input + "\"";

// NEVER: shell execution mode
// std::string cmd = "tgpt -s \"" + input + "\""; // DANGEROUS
```

## Configuration

| Setting | Env Var | Default | Notes |
| :--- | :--- | :--- | :--- |
| Provider path | `TGPT_PATH` | `tgpt` | Allow custom binary location. |
| Timeout | `LLAMA_TIMEOUT` | `120s` | Inherited from global config. |
| Provider | `--provider` | auto | tgpt supports `--provider` flag for backend selection. |

## Open Questions

- [ ] Should we cache provider availability at startup?
- [ ] How to handle provider-specific errors (e.g., rate limits)?
- [ ] Add integration test for tgpt provider in Gherkin specs?
