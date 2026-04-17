# Multi-Model Guide: Ollama & Gemini Integration

This guide documented how `llama-cli` was extended to support multiple AI backends, specifically the Gemini CLI. It explained the core concepts for users who were not experts in AI or C++.

## What was a "Provider"?

In the context of this project, a **Provider** was the engine that generated AI responses. Previously, `llama-cli` only supported **Ollama**. By introducing a provider system, the tool became "model-agnostic," meaning it could talk to different AI services through a unified interface.

1. **Ollama (Local)**: The default provider. It ran entirely on the user's machine, ensuring 100% privacy and offline capability.
2. **Gemini CLI (External)**: An optional provider that leveraged Google's Gemini models via an external command-line tool. It was typically faster and more capable for complex tasks but required an internet connection.

## Why did we support both?

We recognized that users often faced a trade-off between privacy and performance:

| Use Case | Recommended Provider | Reasoning |
| :--- | :--- | :--- |
| **Sensitive Data** | Ollama | Data never left the local machine. |
| **Complex Planning** | Gemini | Larger models handled high-level architecture better than small local ones. |
| **Offline Work** | Ollama | No internet required. |
| **Deep Debugging** | Gemini | Had access to a broader knowledge base for obscure errors. |

## How it was configured

The system was designed to be "zero-config" by default, falling back to Ollama. Users could switch providers using the `LLAMA_PROVIDER` environment variable.

### Using Gemini

The user first ensured the Gemini CLI was installed (e.g., via `brew install gemini`).

```bash
# Set Gemini as the active provider
export LLAMA_PROVIDER=gemini

# Run the tool as usual
make run
```

### Using Ollama (Default)

If no provider was specified, the system defaulted to the local Ollama instance.

```bash
unset LLAMA_PROVIDER
make run
```

## The "Planner/Executor" Workflow

The ultimate goal of this design was to enable a hybrid workflow where:

* **Gemini** acted as the "Planner" (deciding what to do).
* **Ollama** acted as the "Executor" (performing the local file writes and terminal commands).

This allowed for high-intelligence planning without sacrificing the safety of local execution.
