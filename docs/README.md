# Documentation

Llama CLI is a C++ TUI that connects to a local [Ollama](https://ollama.com) instance for interactive LLM conversations.

## Setup

- [Ollama Setup](ollama-setup.md) — install and configure Ollama

## Architecture Decision Records

- [ADR-001: HTTP Library](adr-001-http-library.md) — why cpp-httplib
- [ADR-002: Quality Checks](adr-002-quality-checks.md) — why cppcheck, semgrep, gitleaks, comment ratio
- [ADR-003: Development Workflow](adr-003-v-model-workflow.md) — how we develop features
- [ADR-004: Configuration](adr-004-configuration.md) — env vars + CLI args, no hardcoded values
