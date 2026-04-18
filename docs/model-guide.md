# AI Model & Tool Guide

Which model for which job? A practical guide for this project.

## The Cast (Human-Readable Names)

| Tool / Model | Persona | Strengths | Weaknesses | Cost |
|-------------|---------|-----------|------------|------|
| **kiro-cli** (Claude 4 Sonnet) | "The Senior Architect" — Thinks before coding, writes specs, plans ahead. The one you call for design decisions and complex refactors. | Spec-driven, multi-file reasoning, excellent code quality, reads entire codebases | Free preview may end, requires internet, can be slow on large tasks | Free (preview) |
| **Gemini CLI** (Gemini 2.5 Pro) | "The Eager Mid-Level" — Fast, capable, occasionally overconfident. Good at most things, great at nothing specific. | 1000 req/day free, 1M token context, fast, good general coding | Needs Google account, internet required, free tier limits can change overnight | Free (1000 req/day) |
| **tgpt** (Pollinations/FreeGpt) | "The Unreliable Intern" — Sometimes brilliant, sometimes absent. You never know which model you'll get or if it'll show up at all. | Zero setup, zero cost, access to GPT-5/Claude/Gemini backends | Unreliable, rate limited, no guaranteed model, responses may be truncated | Free (unreliable) |
| **gemma4:e4b** (Ollama, 9.6GB) | "The Reliable Junior" — Always available, follows instructions, but needs hand-holding. Perfect for simple, well-defined tasks with copy-paste prompts. | Always offline, fast, fits most hardware, good at following structured prompts | Struggles with complex multi-file changes, can't reason about architecture | Free (local) |
| **gemma4:26b** (Ollama, 17GB) | "The Thoughtful Mid-Level" — Slower but smarter. Native tool calling, better reasoning. The best local option for this project. | Native function calling, #6 Arena AI, multimodal, Apache 2.0 | Needs 17GB RAM, slower inference, still below cloud models on complex tasks | Free (local) |
| **gemma4-uncensored:e4b** (Ollama, 6.3GB) | "The Unfiltered Junior" — Same as the Junior but won't refuse edge-case requests. Useful when the regular model over-refuses. | No content filtering, smaller footprint | Same limitations as e4b, less tested | Free (local) |

### Models you could install (not currently local)

| Model | Persona | Why consider it | VRAM needed |
|-------|---------|-----------------|-------------|
| **Qwen2.5-Coder 32B** | "The Code Savant" — 92.7% HumanEval. The best local coding model, period. | Best-in-class code generation, refactoring, test writing | 22GB |
| **DeepSeek-R1 32B** | "The Debugger" — Thinks step-by-step through problems. Local o1. | Chain-of-thought reasoning, excellent for debugging complex logic | 20GB |
| **Qwen3-Coder 30B** | "The Agent" — Built for multi-step workflows: read, reason, edit, verify. | RL-trained on SWE-Bench, native tool calling, 256K context | 18GB |
| **Phi-4 14B** | "The Math Nerd" — Beats 70B models on STEM. Microsoft's secret weapon. | Math/STEM specialist, only 10GB VRAM | 10GB |
| **Qwen2.5-Coder 7B** | "The Pocket Coder" — Fits on any GPU, still leads 7B class on coding. | Best coding model for 8GB hardware | 5GB |

---

## Task-to-Model Matrix

### By Story Points / Complexity

| Complexity | Example Tasks | Recommended Tool | Why |
|-----------|---------------|-----------------|-----|
| **1 SP** (trivial) | Fix typo, update config value, rename variable | gemma4:e4b + prompt | Copy-paste prompt, verify, done. No reasoning needed. |
| **2 SP** (simple) | Add git hook, create template file, bump threshold | gemma4:e4b + prompt | Well-defined tasks with exact code in prompt. Junior can handle it. |
| **3 SP** (moderate) | Add CI job, modify existing script, write unit tests | gemma4:26b or Gemini CLI | Needs some reasoning about existing code. Mid-level territory. |
| **5 SP** (complex) | New feature (streaming API layer), refactor module | Gemini CLI or kiro-cli | Multi-file changes, needs to understand architecture. |
| **8 SP** (hard) | Streaming REPL integration, new provider abstraction | kiro-cli | Complex wiring across modules, annotation parsing, edge cases. |
| **13 SP** (epic) | Architecture redesign, new subsystem | kiro-cli + human review | Too complex for any single model. Break into smaller tasks first. |

### By Task Type

| Task Type | Best Tool | Fallback | Why |
|-----------|----------|----------|-----|
| **Execute a prompt** (docs/prompts/) | gemma4:e4b | gemma4:26b | Prompts are self-contained. The Junior can follow instructions. |
| **Write new code** (feat) | Gemini CLI | kiro-cli | Needs to understand context, conventions, existing patterns. |
| **Debug / fix** (fix) | kiro-cli | Gemini CLI | Needs to reason about what went wrong. Senior territory. |
| **Refactor** (chore) | Gemini CLI | gemma4:26b | Needs to understand structure but changes are mechanical. |
| **Write tests** | gemma4:26b | Gemini CLI | Tests follow patterns. Given-When-Then is structured enough for local. |
| **Write docs / ADRs** | Gemini CLI | kiro-cli | Needs broad context and good writing. Mid-level+ territory. |
| **Code review** | kiro-cli | Gemini CLI | Needs to spot subtle issues, understand intent. Senior territory. |
| **Sprint planning** | kiro-cli | Human | Needs strategic thinking, prioritization. Architect territory. |
| **Quick question** | gemma4:e4b | tgpt | Fast answer, no context needed. |
| **Spike / prototype** | tgpt | Gemini CLI | Disposable code, speed over quality. The Intern is fine. |

### By CMMI Level

| CMMI Level | Model Requirement | Why |
|-----------|-------------------|-----|
| **CMMI 0** (Essentials) | gemma4:e4b is sufficient | All tasks have copy-paste prompts. Follow instructions, verify. |
| **CMMI 1** (Managed) | gemma4:26b or Gemini CLI | Some tasks need reasoning (doc-change CI, coverage analysis). |
| **CMMI 2** (Defined) | Gemini CLI or kiro-cli | AI review, mutation testing setup, architecture validation. |
| **CMMI 3** (Optimizing) | kiro-cli | Predictive analysis, cross-team scanning, portfolio decisions. |

---

## Decision Flowchart

```text
Is the task in docs/prompts/?
  ├─ YES → Use gemma4:e4b (the Junior follows instructions)
  └─ NO
      ├─ Is it ≤ 2 story points?
      │   ├─ YES → Use gemma4:e4b or gemma4:26b
      │   └─ NO
      │       ├─ Is it a feat or refactor (3-5 SP)?
      │       │   ├─ YES → Use Gemini CLI (the Mid-Level)
      │       │   └─ NO
      │       │       ├─ Is it complex (8+ SP)?
      │       │       │   ├─ YES → Use kiro-cli (the Architect)
      │       │       │   └─ NO → Use Gemini CLI
      │       │       └─ Is it a spike?
      │       │           └─ YES → Use tgpt (the Intern, disposable)
      └─ Do you need internet?
          ├─ NO → Use gemma4:26b (best local)
          └─ YES → Use Gemini CLI (best free cloud)
```

---

## Provider Comparison

| Aspect | Ollama (local) | Gemini CLI | tgpt | kiro-cli |
|--------|---------------|------------|------|----------|
| **Privacy** | ✅ 100% local | ❌ Google servers | ❌ Unknown servers | ❌ AWS servers |
| **Offline** | ✅ Yes | ❌ No | ❌ No | ❌ No |
| **Reliability** | ✅ Always works | 🔶 Free tier limits | ❌ Unreliable | ✅ Stable (preview) |
| **Speed** | 🔶 Hardware-dependent | ✅ Fast | 🔶 Variable | 🔶 Medium |
| **Code quality** | 🔶 Model-dependent | ✅ Good | 🔶 Variable | ✅ Excellent |
| **Multi-file** | ❌ Weak | 🔶 Moderate | ❌ Weak | ✅ Strong |
| **Tool calling** | ✅ gemma4:26b | ✅ Yes | ❌ No | ✅ Yes |
| **Cost** | Free forever | Free (1000/day) | Free (unreliable) | Free (preview) |
| **Context window** | 128K (gemma4) | 1M | Unknown | Large |
| **Best for** | Prompt execution | General coding | Quick questions | Architecture |

---

## Honest Community Ratings (what people actually say)

| Tool | Community Sentiment | Source |
|------|-------------------|--------|
| **kiro-cli** | "Brilliant architectural ideas, spec-driven approach is unique. Preview is generous. Some find it slow." | Developer reviews, Forbes |
| **Gemini CLI** | "Best free tier in the business. 1000 req/day is generous. Quality is solid but not Claude-level. Google can change limits overnight." | Developer forums, API pricing guides |
| **tgpt** | "Great concept, terrible reliability. Works when it works. Don't depend on it for anything important." | GitHub issues, community feedback |
| **gemma4:e4b** | "Surprisingly capable for its size. MoE architecture is clever. Good for structured tasks, struggles with open-ended coding." | Ollama community, benchmark sites |
| **gemma4:26b** | "#6 on Arena AI is legit. Native function calling works. Best open model for agents under 32B. Needs decent hardware." | Arena AI leaderboard, morphllm.com |
| **Qwen2.5-Coder 32B** | "The local coding king. 92.7% HumanEval is not a typo. If you have 22GB VRAM, install this immediately." | HuggingFace, morphllm.com, continue.dev |

---

## Recommended Setup for This Project

```bash
# 1. Default: always available, handles prompt execution
ollama pull gemma4:e4b          # 9.6GB — "The Reliable Junior"

# 2. Quality: for tasks needing reasoning
ollama pull gemma4:26b          # 17GB — "The Thoughtful Mid-Level"

# 3. Optional: best local coding (if you have 22GB+ RAM)
ollama pull qwen2.5-coder:32b  # 22GB — "The Code Savant"

# 4. Cloud: for complex features and architecture
# Install Gemini CLI: https://github.com/google-gemini/gemini-cli
# Install kiro-cli: https://kiro.dev

# 5. Quick & dirty: for spikes and throwaway questions
# Install tgpt: brew install tgpt
```

## References

- [morphllm.com — 12 Ollama Models Ranked](https://www.morphllm.com/best-ollama-models) (April 2026)
- [Arena AI Leaderboard](https://lmarena.ai/) — Gemma 4 26B at #6
- [Gemini CLI Quotas](https://geminicli.com/docs/resources/quota-and-pricing/) — 1000 req/day free
- [continue.dev — Top Ollama Coding Models](https://resources.continue.dev/top-ollama-coding-models-q4-2025)
- [HuggingFace Open LLM Leaderboard](https://huggingface.co/spaces/open-llm-leaderboard/open_llm_leaderboard)
