---
summary: Ensure all TUI output is accessible — never rely on color alone for status.
status: accepted
---

# ADR-123: Accessible TUI Output

## Context

cpm uses colored output (green ✓, red ✗) for status. This fails for:
- Color-blind users (8% of men, 0.5% of women)
- Terminals without color support
- Screen readers
- Piped output / log files

## Decision

**Never use color as the only indicator.** Every status has a symbol AND a label.

### Status indicators (color + symbol + optional label)

| Status | Symbol | Color | Label (NO_COLOR mode) |
|--------|--------|-------|----------------------|
| Pass | ✓ | green | `pass` |
| Warning | ⚠ | yellow/orange | `warn` |
| Error | ✗ | red | `fail` |
| Skip | ⊘ | gray | `skip` |
| Slow (>5s) | ✓ | yellow | `slow` |
| Very slow (>10s) | ✓ | red | `SLOW` |
| Info | ℹ | blue | `info` |

### Timing thresholds (visual feedback on performance)

```text
  format-code  ✓ 400ms          ← green: fast
  lint-code    ✓ 8.2s [slow]    ← yellow: getting slow
  check-xref   ✓ 14.1s [SLOW]   ← red: needs optimization
```

Thresholds configurable in cpm.toml:

```toml
[ui.thresholds]
slow = 5       # seconds → yellow
very-slow = 10 # seconds → red
```

### NO_COLOR compliance

When `NO_COLOR` is set or output is not a TTY:
- No ANSI escape codes
- Status shown as text label: `[pass]`, `[fail]`, `[skip]`, `[slow]`
- Symbols still shown (Unicode, not color-dependent)

### Inclusive thinking (not just policy)

Accessibility is not a checkbox. The principle:

> **If you remove all color from the output, can you still understand the status?**

If yes → accessible. If no → fix it.

This applies to:
- Terminal output (cpm TUI)
- JUnit XML (no color, structured data)
- Log files (no color, readable)
- CI output (often no color)

## Implementation

<!-- TODO: update timer_stop in timer.sh to show [slow]/[SLOW] based on thresholds -->
<!-- TODO: update print_step in ui.sh to include text label in NO_COLOR mode -->
<!-- TODO: audit all print_* functions for color-only status indicators -->

## Consequences

- Output is readable by everyone regardless of vision
- NO_COLOR standard respected (https://no-color.org/)
- Performance issues visible at a glance (yellow/red timing)
- No additional dependencies

## References

- https://no-color.org/ (standard for disabling color)
- @see lib/cpm/shell/ui.sh (implements NO_COLOR)
- @see docs/adr/adr-121-cpm-quality-layer.md (output layer)
