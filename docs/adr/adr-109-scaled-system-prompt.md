---
summary: The document proposes scaling system prompts based on model size for small language models (≤7B parameters), reducing hallucinations of tool calls by adopting minimal prompts with ≤7 billion parameters.
status: accepted
---

# ADR-109: Scaled System Prompt by Model Size

## Context

Small models (≤7B parameters like llama3.2:3b) receive the same complex system prompt as large models (26B+). The full prompt includes instructions for `<exec>`, `<read>`, `<write>`, `<str_replace>`, mermaid diagrams, and agent delegation. Small models misinterpret these — they hallucinate tool calls like `<exec>math 1+1</exec>` or `<read path="message"/>` for simple questions.

## Decision

Scale the system prompt based on model size tag in the model name:

- **Small** (`:1b`, `:3b`, `:4b`, `:7b`): Minimal prompt — answer directly, only use `<exec>` when explicitly asked
- **Large** (everything else): Full prompt with all tool instructions

The small prompt (`res/system-prompt-small.txt`) is 3 lines vs ~40 lines for the full prompt.

Selection is overridden if the user explicitly sets `--system-prompt` or `OLLAMA_SYSTEM_PROMPT`.

## Consequences

- Small models stop hallucinating tool calls for simple questions
- "wat is 1+1?" gets a direct "2" instead of `<exec>math 1+1</exec>`
- Users can still force the full prompt with `--system-prompt` if needed
- Model size detection is heuristic (tag-based), not API-based — covers 99% of Ollama models
