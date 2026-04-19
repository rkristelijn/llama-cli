# 021: Prompt Templates

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#85](https://github.com/rkristelijn/llama-cli/issues/85)

## Problem

The system prompt is static and generic. Different tasks (code review, debugging, writing) benefit from different prompt structures. Users re-type the same framing every session.

## Idea

Switchable prompt templates that set the system prompt + parameters for a task type:

```bash
/template code-review    # "Act as senior C++ developer, focus on bugs and security"
/template debug          # "Analyze errors systematically, check logs first"
/template write          # "Write concise, clear prose"
/template default        # Reset to built-in system prompt
```

### Template structure

```json
{
  "name": "code-review",
  "system_prompt": "You are a senior C++ developer reviewing code...",
  "temperature": 0.2,
  "description": "Focused code review with security and performance analysis"
}
```

### Storage

- Built-in templates ship with the binary
- User templates in `~/.llama-cli/templates/`
- `/template list` shows all available

### Research: CRISP framework

Effective prompts follow: **C**ontext → **R**ole → **I**nstruction → **S**pecification → **P**arameters. Templates encode this structure so users don't have to.

## References

- [ADR-004: Configuration](../adr/adr-004-configuration.md)
- [020 — Temperature tuning](020-temperature-tuning.md) — templates set temperature per task
