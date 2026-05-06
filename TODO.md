
# TODO

- [ ] Optimize writes by using small updates.
- [ ] Fix bug: Input breaking on mobile/telephone (duplicate lines on every keypress and backspace).
- [ ] Cache `make check` steps: skip checks when no relevant files changed since last pass (hash-based, per target).
- [ ] Theme support: define color scheme in .env (e.g., LLAMA_THEME=dark/light/custom) instead of individual color settings. Single theme name maps to prompt_color, ai_color, system_color, etc.
- [ ] String literals audit: UI messages are scattered as inline strings. Consider a central `messages.h` or i18n-ready string table if we ever need localization. For now, keep provider/model names dynamic (from registry scan, never hardcoded).
- [ ] Confirmation feedback: allow typing a reason when declining (e.g., "n, use sed instead"). Feed the feedback back as a user message so the LLM retries with the correction.
- [ ] Tokens/sec tracking: measure actual tokens/sec from Ollama response, store rolling average in `.tmp/model-stats.json`, show in `/model` list (replacing static estimates). Add `/bench reset` to clear stats.
- [ ] lets forget about making bak files outside of git, maybe make ext._ or something idk
- [ ] pagination bij overflow adhv consloe height, en strikt patter met nummers of beter letters, geen nummers bij keuze
- [ ] onderzoek doen naar tui design best practices en a11y
