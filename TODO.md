
# TODO

- [ ] Optimize writes by using small updates.
- [ ] Fix bug: Input breaking on mobile/telephone (duplicate lines on every keypress and backspace).
- [ ] String literals audit: UI messages are scattered as inline strings. Consider a central `messages.h` or i18n-ready string table if we ever need localization. For now, keep provider/model names dynamic (from registry scan, never hardcoded).
- [ ] Confirmation feedback: allow typing a reason when declining (e.g., "n, use sed instead"). Feed the feedback back as a user message so the LLM retries with the correction.
- [ ] Tokens/sec tracking: measure actual tokens/sec from Ollama response, store rolling average in `.tmp/model-stats.json`, show in `/model` list (replacing static estimates). Add `/bench reset` to clear stats.
- [ ] lets forget about making bak files outside of git, maybe make ext._ or something idk
- [ ] pagination bij overflow adhv consloe height, en strikt patter met nummers of beter letters, geen nummers bij keuze
- [ ] Delegation warmup: consider sending keep_alive ping to delegation targets on startup, or set OLLAMA_NUM_PARALLEL=2 on M2 to keep multiple models loaded. Currently async delegation absorbs the cold-start cost invisibly.
- [ ] Terminal pixel rendering for mermaid diagrams: use half-blocks (▀▄█) for solid shapes + braille for thin lines. Gives 80×80 "pixel" canvas in standard terminal. Prototype in ../tui-mermaid. See ADR-102 for allowed characters.
