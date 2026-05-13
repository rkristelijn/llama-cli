---
summary: The scripts provide an idempotent and OS-aware installation process for various CLI LLM tools and infrastructure on Linux and macOS, leveraging curl piping, while covering tools like llama-cli, kiro-cli, and Docker/SearXNG containers.
status: accepted
---

# ADR-113: Batch Install Scripts for LLM Tools and Infrastructure

## Context

Setting up a development environment with multiple terminal AI tools requires visiting different documentation pages and running different install commands per tool and per OS. This friction slows onboarding and makes it hard to reproduce a consistent "AI-powered terminal" setup across machines.

llama-cli already has a one-liner install script. We want the same convenience for the broader ecosystem of CLI LLM tools, and for the Docker/SearXNG infrastructure that powers llama-cli's web search feature.

## Decision

Add four curl-pipeable scripts to `scripts/`:

| Script | Purpose |
|--------|---------|
| `install-llm-tools-linux.sh` | Install all CLI LLM tools on Linux |
| `install-llm-tools-mac.sh` | Install all CLI LLM tools on macOS (Homebrew) |
| `install-docker-searxng-linux.sh` | Docker Engine + SearXNG container |
| `install-docker-searxng-mac.sh` | Docker Desktop + SearXNG container |

### Tools covered

llama-cli, kiro-cli, amazon-q, opencode, tgpt, gemini-cli, copilot-cli, claude-code, aider

### Design principles

- **Idempotent**: skip already-installed tools
- **OS-aware**: Linux uses native installers/AppImage; macOS uses Homebrew
- **Transparent**: warn about tools that redirect (Amazon Q → Kiro) with fallback URLs
- **No sudo by default**: only where unavoidable (Docker, .deb install)

## Consequences

### Positive

- One-command setup for a full AI terminal environment
- Reproducible across machines (Linux workstation, MacBook)
- Scripts double as documentation of current install methods
- Distributable via raw GitHub URL or gist

### Negative

- Install methods change upstream; scripts need periodic updates
- Some tools require post-install authentication (not automatable)
- Amazon Q install path is unstable due to Kiro migration
