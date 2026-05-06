# ADR-091: Free ChatGPT Integration via tgpt CLI Provider

**Status:** Proposed  
**Date:** 2026-05-06
**Context:** Many users want ChatGPT or GPT-3/4 quality in their local AI stack, but without needing (or leaking) a paid OpenAI API key. Popular tools (e.g. OpenCode, Open Interpreter) achieve this by integrating free community-driven providers like [tgpt](https://github.com/aandrew-me/tgpt).

---

## Decision

Integrate ChatGPT/GPT-like functionality as a "free provider" using the `tgpt` CLI tool:

- tgpt is a cross-platform CLI leveraging free/rotating community backends (Pollinations.AI, FreeGpt, Poe, etc.)
- Integration fits our existing Provider Abstraction (see [ADR-020](adr-020-provider-abstraction.md))
- No OpenAI/official API key is ever needed; just install tgpt and go.

## Problem Statement

While local (Llama, Gemma, etc.) models ensure privacy, many users expect access to ChatGPT or GPT-4 quality, for specific scenarios, quick tests, or AI benchmarking. Using a paid OpenAI API is not always possible or desirable. Scraping official web interfaces is ethically/legally fragile and technically brittle, but CLI bridges such as tgpt are widely used.

## Why tgpt?

- Used as the de facto "free ChatGPT bridge" in OpenCode, Open Interpreter and other OSS agents
- Prompt/pipe pattern, selectable backends/providers, robust CLI output, error reporting
- One-line installation (`brew install tgpt`, `go install ...`, install script, etc.)
- No login, no secret required (uses community APIs)
- Integrates perfectly with legacy exec/REPL abstractions

## Implementation Pattern

- On provider selection or at REPL startup: detect if `tgpt` is available (`which tgpt` or PATH lookup)
- Add `TgptProvider` or `provider: tgpt` option to abstraction (see ADR-020 for class/enum)
- Call tgpt with collapsed conversation as a single prompt:

  ```text
  tgpt -p "<prompt>"
  ```

  and for file input:

  ```text
  cat file.txt | tgpt "summarize this"
  ```

  See [ADR-031](adr-031-tgpt-provider.md) for details.
- Capture output, handle shell escaping, separate stdout (LLM reply) and stderr (errors)
- Make it selectable as any other provider (flag/ENV/config)
- (Optional) Display prominent info that this is a community/dependency provider ("best effort, not OpenAI official")

## Alternatives Considered

| Alternative                    | Why Not Used                    |
|-------------------------------|---------------------------------|
| Native OpenAI API             | Requires paid/real key          |
| Webscraping/headless browser  | Brittle, slow, ToS-violating    |
| Local models only             | No ChatGPT parity/fun/benchmarks|
| Custom relay proxy            | Complex, overkill for OSS       |

## Consequences

**Positives:**

- Provides free ChatGPT/GPT-like access to all users with no login
- Easy CLI/script integration
- Fast onboarding, broad OSS adoption

**Negatives:**

- Community backends can change, rate limit, or go offline
- No guarantees on privacy (community hosts!) or ethics; show warning if needed
- Not for critical or sensitive workloads

## Usage/Docs

- Document clearly that this is a "free, unofficial GPT bridge"; not OpenAI or privacy-grade
- Advise local models for privacy/critical content
- Show provider info ("tgpt: free ChatGPT-quality, via community OSS hosts") in UI/CLI

## References

- [tgpt repo](https://github.com/aandrew-me/tgpt)
- [ADR-031: tgpt-provider](adr-031-tgpt-provider.md)
- [ADR-020: provider abstraction](adr-020-provider-abstraction.md)
- (How OpenCode and Open Interpreter integrate tgpt)

## Open Questions

- Should we cache tgpt availability on startup?
- Should we fallback to local LLM if tgpt unavailable?
- How are provider errors (rate limit, context) surfaced to user?
