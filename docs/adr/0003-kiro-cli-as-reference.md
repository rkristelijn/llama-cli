# ADR-003: Use Kiro CLI as Reference Implementation

**Date:** 2026-04-15  
**Status:** Accepted

## Context

We are building a CLI tool for personal life management with AI assistance. During research, we discovered [Kiro CLI](https://github.com/kirodotdev/Kiro), an open-source AI-powered CLI tool.

## Decision

We will use Kiro CLI as a reference implementation for architectural patterns and best practices.

## Reasoning

1. **Open Source** — Kiro CLI is fully open source (TypeScript), allowing us to study its implementation
2. **Similar Scope** — Both tools provide AI-assisted CLI interactions
3. **Mature Project** — Kiro CLI is actively maintained with 3.4k+ stars
4. **Reference Use Cases**:
   - Slash commands (`/usage`, `/model`, `/chat`)
   - Agent steering and configuration
   - Session management
   - MCP integration patterns
   - TUI/terminal interactions

## Implementation Language

Kiro CLI is written in **TypeScript** (primary), with some JavaScript and Shell.

## References

- Repository: https://github.com/kirodotdev/Kiro
- Docs: https://kiro.dev/docs/cli/
- License: Proprietary/Commercial (source available for reference)

## Notes

- We are not forking or copying code directly
- Study patterns, then implement our own solutions
- Respect Kiro's licensing and terms of use