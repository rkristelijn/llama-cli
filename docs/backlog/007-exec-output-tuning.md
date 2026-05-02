# 007: Exec Output Tuning

*Status*: ✅ Done · *Date*: 2026-04-19 · *Issue*: [#27](https://github.com/rkristelijn/llama-cli/issues/27)

## Problem

After `<exec>` output is fed back to the LLM, it tends to repeat the output verbatim instead of interpreting it. Wastes tokens and provides no value.

## Solution (implemented)

1. **System prompt instruction**: "When you receive command output, ANALYZE it — do not repeat it verbatim." (see `config.h`)
2. **Output truncation**: `LLAMA_MAX_OUTPUT` (default 10000 chars) truncates large outputs with a `[truncated at N chars]` marker before feeding back to the LLM (see `exec.cpp`)
3. **Configurable**: `--max-output` CLI flag, `LLAMA_MAX_OUTPUT` env var, `.env` support

### References

- [ADR-015: Command Execution](../adr/adr-015-command-execution.md)
- [ADR-014: Tool Annotations](../adr/adr-014-tool-annotations.md)
