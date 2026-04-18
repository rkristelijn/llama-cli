---
title: "feat: add streaming responses"
labels: feat, sprint-2
---

## Why (Value Statement)

Streaming is the last unchecked roadmap feature. Without it, users stare at a blank screen during long responses — poor UX that undermines the "Fast" value proposition. Streaming shows tokens as they arrive, making the tool feel responsive.

## ISO 9126 Quality Target

- [x] Usability
- [x] Efficiency

## Acceptance Criteria

```gherkin
Given a user sends a prompt in interactive mode
When the LLM generates a response
Then tokens appear on screen as they are generated (not all at once)

Given a user presses Ctrl+C during streaming
When the stream is interrupted
Then the partial response is discarded and the prompt returns

Given a response contains <write> or <exec> annotations
When streaming completes
Then annotations are parsed from the full response and processed normally
```

## Implementation

This feature is split into 3 sub-tasks (prompts), to be executed in order:

1. **[09-streaming-ollama.md](../../docs/prompts/09-streaming-ollama.md)** — Add `ollama_chat_stream()` to the API layer
2. **[10-streaming-repl.md](../../docs/prompts/10-streaming-repl.md)** — Wire streaming into the REPL loop
3. **[11-streaming-tests.md](../../docs/prompts/11-streaming-tests.md)** — Add unit tests for streaming

## RAID

- **Risks**: Annotation parsing must happen after full response — partial XML tags during streaming could confuse users
- **Assumptions**: cpp-httplib content receiver works with Ollama's chunked JSON
- **Dependencies**: Prompts must be executed in order (09 → 10 → 11)

## References

- README.md Roadmap: "Streaming responses"
- ADR-048 §15 Sprint 2 Plan
- Prompts: `docs/prompts/09-streaming-ollama.md`, `10-streaming-repl.md`, `11-streaming-tests.md`
