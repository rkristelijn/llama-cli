---
summary: FlyAI to replace llama-cli with a new identity featuring punny branding, while exploring synergy with Opencode for enhan
status: accepted
---
summary: FlyAI to replace llama-cli with a new identity featuring punny branding, while exploring synergy with Opencode for enhan

# ADR-092: Product Renaming & Integration Strategy with Opencode

## Context

The project, previously known as `llama-cli`, lacks a unique, memorable, and brandable name that fits its modern, nerdy, and privacy-oriented terminal AI persona. The goal is to select a new identity and set a strategic vision for possible synergy with advanced AI agents such as Opencode.

## Decision

### 1. **Product Renaming**

- The new product name will be: **FlyAI**
  - Inspired by the pun "Pretty Fly for an AI" (as in the classic Offspring lyric, with a geek twist)
  - Short, memorable, unique, and fun
  - CLI binary: `flyai` (and optionally `fly`)
  - Session or CLI banner/prompt should by default include the tagline: `pretty fly for an AI`
- All references to `llama-cli` should transition to `FlyAI` in docs, output, help text, etc.
- Motivation:
  - Uniqueness in open source/CLI context
  - Instantly nerdy, playful, and memorable
  - Allows for playful ASCII art and memorable branding

### 2. **Prompt/Banner Update**

- On each startup (REPL or chat session), FlyAI will show (as a header or in the prompt):

    ```text
    pretty fly for an AI
    ```

- This serves both as an icebreaker and branding.

### 3. **Opencode Strategy & Synergy**

- FlyAI will actively explore synergy with fully autonomous AI agents like Opencode.
- Integration & inspiration options:
  - Support for "agent provider" strategy (make it easy to call Opencode as a backend or provider from within FlyAI)
  - Use Opencode’s agent/task/todo and architecture patterns as inspiration for advanced workflows (e.g. planner/executor patterns, skill registry, autonomous todo breakdown)
  - Consider sharing or adapting prompt libraries, especially for complex code-editing, planning and review tasks
  - Allow (now or in future) for FlyAI to delegate tasks, or receive autonomous code changes/to-dos/PRs from Opencode or similar agents (optionally with user confirmation)
  - Cross-reference and learn from Opencode’s ADRs and implementation choices to stay modular, extensible, and competitive
- Strategic motivation:
  - Focus FlyAI on being the local, privacy-first, nerd-friendly, shell-integrated AI sidekick
  - Leverage more autonomous systems (like Opencode) for complex, multi-step coding and software agent workflows, optionally as an add-on or service backend
  - Accelerate development/innovation by building on best-of-breed agent and workflow patterns from the open source AI tools ecosystem

### 4. **Logging & Usage Analytics**

- FlyAI will continue to offer detailed logging of actions, prompts, command and model usage, and timeline/session statistics.
- Built-in `/usage` commands, real-time stats (tokens per minute, provider/model breakdown, most used commands, etc.), and session logging will remain a unique FlyAI feature.
- These analytics support transparency, personal tracking, performance tuning, and responsible AI use.

## Consequences

- The product will standardize on the name FlyAI.
- All branding, documentation, and command-line interfaces will reflect this new identity.
- The prompt/banner will give a recognizable, fun first impression.
- Opencode’s agent and workflow architecture will be actively studied for inspiration and possible integration.
- FlyAI will keep its focus: local, user-in-control, privacy-first, but will seek synergy in agent/planner/todo/skill features and interfaces.

---
