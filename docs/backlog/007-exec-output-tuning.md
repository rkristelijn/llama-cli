# 007: Exec Output Tuning

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#27](https://github.com/rkristelijn/llama-cli/issues/27)

## Problem

After `<exec>` output is fed back to the LLM, it tends to repeat the output verbatim instead of interpreting it. Wastes tokens and provides no value.

## Idea

Tune the system prompt to instruct the LLM to analyze exec output, not parrot it. Possible additions:

- "When you receive command output, summarize findings — do not repeat the output."
- Truncate large outputs before feeding back (keep first/last N lines)
- Add a `[command output — analyze, don't repeat]` wrapper around exec results

### References

- [ADR-015: Command Execution](../adr/adr-015-command-execution.md)
- [ADR-014: Tool Annotations](../adr/adr-014-tool-annotations.md)
