# 005: Streaming Responses

*Status*: Idea · *Date*: 2026-04-19 · *Issues*: [#4](https://github.com/rkristelijn/llama-cli/issues/4), [#25](https://github.com/rkristelijn/llama-cli/issues/25)

## Problem

The LLM response is buffered until complete, causing a perceived delay. Users stare at a spinner instead of seeing tokens arrive.

## Idea

Stream tokens as they arrive from Ollama's `/api/chat` (which supports `"stream": true` by default). Eliminates the spinner wait and gives instant feedback.

### Challenges

- cpp-httplib's POST doesn't natively support chunked response streaming
- Annotation parsing (`<write>`, `<exec>`, `<str_replace>`) must work on partial output
- Ctrl+C interrupt mid-stream needs clean teardown

### References

- [ADR-001: HTTP Library](../adr/adr-001-http-library.md)
- [Ollama streaming API](https://github.com/ollama/ollama/blob/main/docs/api.md)
