# Using AI Efficiently

Practical strategies for getting the most out of paid AI tools like Kiro, while offloading to free alternatives where possible.

---

## 1. Know your budget

Kiro Free gives 50 credits/month (no overages). Credits reset on the 1st.

| Tier   | Credits | Overage |
|--------|---------|---------|
| Free   | 50      | No      |
| Pro    | 1,000   | Opt-in  |
| Pro+   | 2,000   | Opt-in  |
| Power  | 10,000  | Opt-in  |

Credits are fractional — short prompts cost less than long, complex ones.

Check your usage:
```bash
# scripts/check-usage.sh or just look at usage.log
cat usage.log
```

---

## 2. Use free alternatives first

Before reaching for Kiro, consider:

- **Local LLM via Ollama** — this project runs `gemma4:e4b` locally. Zero cost, fully private.
  ```bash
  make run   # starts llama-cli with local model
  ```
- **Gemini CLI** (`gemini -p`) — free tier, good for design work, brainstorming, or parallel tasks while waiting on a paid tool. Use it for drafts, then refine with Kiro.
- **`!!cat file`** inside llama-cli — feeds file content to the local LLM, no credits spent.

Rule of thumb: use local/free for exploration, use Kiro for precise execution.

---

## 3. Automate repetitive tasks with scripts

Every task you do manually more than twice should become a script. Scripts give idempotent, reproducible results — no need to re-explain context to an AI.

Examples in this repo:
- `scripts/build-index.sh` — regenerates INDEX.md, always consistent
- `scripts/pipeline-status.sh` — checks CI without opening a browser
- `scripts/pr-status.sh` — shows failed jobs with log output

**Idempotency matters**: a script that produces the same result every time means you don't need to ask an AI "what changed?" — you just run it.

---

## 4. Give AI the right context upfront

The more precise your input, the fewer back-and-forth rounds (each round costs credits).

- **Give file paths directly**: `fix the bug in src/repl/repl.cpp:145` beats "there's a bug somewhere in repl"
- **Paste actual output**: paste error messages or command output instead of describing them (`pbpaste` workflow)
- **Combine related tasks**: "move scripts, fix the pager bug, update the index" in one message is cheaper than three separate messages
- **Reference the index**: `INDEX.md` gives AI a map of the repo — keep it up to date so you can say "see the index" instead of explaining structure

---

## 5. Keep context small

Long conversations accumulate context and cost more per message.

- Start a new chat for unrelated tasks
- Use `!!cat` for targeted file reads instead of pasting entire files
- After a big task, summarize and start fresh

---

## 6. Use the right model

Kiro lets you select models with different credit multipliers:

| Model           | Multiplier | Good for                        |
|-----------------|------------|---------------------------------|
| minimax-m2.1    | 0.15x      | Simple edits, formatting        |
| deepseek-3.2    | 0.25x      | Routine coding tasks            |
| claude-haiku    | 0.40x      | Fast, lightweight reasoning     |
| auto            | 1.00x      | General use                     |
| claude-sonnet-4 | 1.30x      | Complex reasoning, architecture |

Use a cheaper model for mechanical tasks (renaming, formatting, moving files).

---

## 7. Optimize your docs for AI consumption

The markdown files in this repo double as AI context. Write them so an AI can act on them without clarification:

- Clear headings and structure
- Explicit file paths and commands
- ADRs that explain *why*, not just *what*
- `INDEX.md` as a navigation layer

A well-structured `docs/` means you can say "read the architecture doc and implement X" and get a useful result in one shot.

---

## 8. Parallel workflows

Don't wait idle. While Kiro is working on one task:
- Use `gemini -p` to draft a design or write docs
- Run local builds or tests
- Use the local LLM for exploration

AI tools work best when used in parallel, not sequentially.
