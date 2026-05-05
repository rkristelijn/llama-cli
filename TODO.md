
# TODO

- [ ] Optimize writes by using small updates.
- [ ] Fix bug: Input breaking on mobile/telephone (duplicate lines on every keypress and backspace).
- [ ] Cache `make check` steps: skip checks when no relevant files changed since last pass (hash-based, per target).
- [ ] Theme support: define color scheme in .env (e.g., LLAMA_THEME=dark/light/custom) instead of individual color settings. Single theme name maps to prompt_color, ai_color, system_color, etc.
