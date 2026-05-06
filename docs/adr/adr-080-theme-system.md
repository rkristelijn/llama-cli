# ADR-080: Terminal Theme System

*Status*: Accepted · *Date*: 2026-05-05

## Context

Colors were hardcoded as ANSI escape sequences throughout `src/tui/`. This made customization impossible and created maintenance burden. Users want to personalize their terminal experience.

## Decision

Adopt a **role-based theme system** inspired by MUI's palette architecture, adapted for terminal ANSI output.

### Architecture

```text
Theme (named preset)
  └── Role (semantic purpose)
       └── Style (visual properties)
            ├── fg: foreground color name
            ├── bg: background color name
            ├── bold: bool
            ├── italic: bool
            ├── underline: bool
            └── dim: bool
```text

### Roles (semantic, not visual)

| Role | Purpose | Default (dark) |
|------|---------|----------------|
| prompt | User input prompt (nick>) | bold green |
| ai | AI response text | default |
| system | System messages, status | dim |
| error | Error messages | bold red |
| info | Command output, help | cyan |
| warning | Proposals, confirmations | yellow |
| banner | Startup ASCII art | bold yellow |
| code | Inline code in markdown | cyan |

### Built-in themes

| Theme | Description |
|-------|-------------|
| dark | Default — green prompt, subtle system messages |
| light | Blue prompt, gray AI — for light terminals |
| mono | No colors — accessibility, piping, CI |
| hacker | All green — matrix aesthetic |

### User customization

```bash
# In REPL:
/theme hacker                              # switch preset
/theme set prompt bright_cyan bold underline  # customize one role
/theme set ai green italic                   # green italic AI responses
/theme set error red bold bg:white           # red on white background

# In .env (persisted):
LLAMA_THEME=hacker
```text

### Design principles (from MUI)

1. **Role-based, not color-based**: code references `theme.error`, never `\033[31m`
2. **Defaults + overrides**: built-in themes provide sensible defaults; users override individual roles
3. **Separation of concerns**: Theme defines *what* colors mean; Style renders *how* they appear
4. **No hardcoded ANSI in business logic**: all color output goes through `Style::ansi()` + `Style::reset()`
5. **Progressive customization**: pick a preset, then tweak individual roles as needed

### Implementation

- `src/tui/theme.h` — `Style` struct, `Theme` struct, built-in presets, `active_theme()` singleton
- `src/tui/tui.h` — all output functions use `active_theme()` roles
- `/theme` command — list, switch, customize roles at runtime
- `.env` persistence via `LLAMA_THEME` key

### Migration path

Phase 1 (now): tui.h functions use theme. Prompt uses theme.
Phase 2 (later): markdown renderer uses theme.code, theme.dim for fenced blocks.
Phase 3 (later): spinner, table renderer use theme roles.

## Consequences

- No more hardcoded ANSI codes in new code (enforced by code review)
- Users can fully customize their terminal experience
- Accessibility: mono theme works for screen readers and piped output
- Theme can be persisted in .env for cross-session consistency
- Markdown renderer still has some hardcoded codes (Phase 2 migration)
