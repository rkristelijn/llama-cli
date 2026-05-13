
# TODO

- [ ] Fix bug: Input breaking on mobile/telephone (duplicate lines on every keypress and backspace).
- [ ] Confirmation feedback: allow typing a reason when declining (e.g., "n, use sed instead"). Feed the feedback back as a user message so the LLM retries with the correction.
- [ ] Tokens/sec tracking: measure actual tokens/sec from Ollama response, store rolling average in `.tmp/model-stats.json`, show in `/model` list (replacing static estimates). Add `/bench reset` to clear stats.
- [ ] Pagination: use letters instead of numbers for model selection (avoid confusion with line numbers).
- [ ] Delegation warmup: consider sending keep_alive ping to delegation targets on startup, or set OLLAMA_NUM_PARALLEL=2 on M2 to keep multiple models loaded. Currently async delegation absorbs the cold-start cost invisibly.
- [ ] Terminal pixel rendering for mermaid diagrams: use half-blocks (▀▄█) for solid shapes + braille for thin lines. Gives 80×80 "pixel" canvas in standard terminal. Prototype in ../tui-mermaid. See ADR-102 for allowed characters.
- [ ] C-style casts: 51 files use C-style casts (ES.48). Fix with `static_cast<T>()`. Use `scripts/lint/fix-casts.sh` when clang-tidy is in PATH. Do in batches of 5-10 files per PR.
- [ ] Unified script output format: standardize all script output through a single TUI helper (like ../workspace-tui/). Replace ad-hoc echo/printf with consistent formatting functions for headers, success, warnings, errors, progress bars, and summaries. One source of truth for output styling across all scripts.
