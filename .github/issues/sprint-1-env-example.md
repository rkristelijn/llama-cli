---
title: "chore: add .env.example configuration template"
labels: chore, sprint-1, cmmi-0
---

## Why (Value Statement)

New contributors have no reference for required configuration (C4I — Code for Inclusivity). The README mentions `--default-env > .env` but there's no committed example. This blocks onboarding and violates the KT Rule: "if a dev can't get started without asking, the docs have a bug."

## ISO 9126 Quality Target

- [x] Usability

## Acceptance Criteria

```gherkin
Given a new contributor clones the repo
When they look for configuration guidance
Then they find .env.example with all settings documented

Given a contributor runs "cp .env.example .env"
When they run "make build && ./build/llama-cli"
Then the tool starts with sensible defaults
```

## Implementation

Create `.env.example` from the Configuration table in README.md:
```bash
# Ollama connection
OLLAMA_HOST=localhost
OLLAMA_PORT=11434
OLLAMA_MODEL=gemma4:e4b
OLLAMA_TIMEOUT=120

# Execution limits
LLAMA_EXEC_TIMEOUT=30
LLAMA_MAX_OUTPUT=10000
```

## Stacking Effects

1. → Onboarding time reduction (C4I compliance)
2. → Documents all config in one place (single source of truth)

## RAID

- **Risks**: None
- **Dependencies**: None

## References

- ADR-048 §10 C4I principle
- ADR-048 §11 Knowledge Management
- ADR-004 Configuration
