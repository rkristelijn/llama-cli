# ADR-021: Gemini CLI Integration

*Status*: Rejected · *Date*: 2026-04-12 · *Context*: We needed to implement the `GeminiProvider` following the abstraction defined in ADR-020. This provider used the existing `gemini` CLI tool installed on the user's system.

## Decision

The `GeminiProvider` was implemented by invoking the `/opt/homebrew/bin/gemini` process in non-interactive (headless) mode using the `--prompt` (or `-p`) flag.

### Interaction Pattern

Unlike Ollama, which used an HTTP API with a structured JSON history, the Gemini CLI was designed primarily for direct terminal interaction. To use it as an engine, we followed these steps:

1. **Format History**: We collapsed the conversation history into a single string.
2. **Invoke Subprocess**: We called `gemini -p "<formatted_prompt>"` using the `exec` module.
3. **Capture Output**: We captured the `stdout` and returned it to the REPL.

### Rationale

1. **Zero Dependency**: By calling the CLI tool, we avoided adding complex Google Cloud SDKs or managing API keys directly in C++.
2. **Leveraging Existing Code**: We reused the `exec` module (ADR-015), which already handled timeouts and output truncation.
3. **Simplicity**: This approach allowed for a "stateless" integration that was easy to implement and debug.

## Implementation Details

### 1. History Collapse (Prompt Engineering)

Since the Gemini CLI (headless) did not support a "history" object, we concatenated the conversation:

```cpp
// Pseudocode for history collapse
std::string collapsed = "";
for (const auto& msg : history) {
    collapsed += "[" + msg.role + "]: " + msg.content + "\n";
}
collapsed += "[assistant]: ";
```

### 2. Execution Strategy

We used `cmd_exec` from `src/exec/exec.h` to run the command:

```cpp
// The shell command built by GeminiProvider
std::string cmd = "/opt/homebrew/bin/gemini -p \"" + escape(collapsed) + "\"";

// Reusing ADR-015 logic
ExecResult res = cmd_exec(cmd, cfg.timeout, cfg.max_output);
```

### 3. Escaping and Safety

To prevent "shell injection," we added a robust escaping function to ensure that user input containing quotes or special characters wouldn't break the `gemini` command.

## Alternatives Considered

| Alternative | Why Not |
| :--- | :--- |
| **Direct REST API** | Required manually managing OAuth/API tokens and complex JSON structures. |
| **Interactive Mode (REPL-to-REPL)** | Hard to control programmatically; scraping output from an interactive TUI was unreliable. |

## Consequences

* **Dependency**: The user must have `gemini` installed and configured in their PATH.
* **Latency**: Starting a new process for every message was slightly slower than a persistent HTTP connection (Ollama), but acceptable for high-intelligence tasks.
* **Prompt Length**: Extremely long histories could hit OS limits for command-line arguments (though `10,000` chars was well within the `ARG_MAX` on macOS/Linux).
