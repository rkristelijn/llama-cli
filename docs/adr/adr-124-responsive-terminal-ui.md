---
summary: Responsive terminal UI with breakpoints, V-model visualization, and accessible status indicators.
status: proposed
---

# ADR-124: Responsive Terminal UI

## Context

Terminal sizes vary wildly: iPhone (67x37), laptop (120x40), ultrawide (240x60). Current output assumes wide terminals. Need responsive layout like CSS breakpoints.

## Decision

### Terminal breakpoints

| Size | Columns | Behavior |
|------|---------|----------|
| `xs` | <60 | Minimal: symbol + name only, no timing |
| `sm` | 60-79 | Compact: symbol + name + time, truncate long names |
| `md` | 80-119 | Standard: full output (current default) |
| `lg` | 120-159 | Extended: add trend, add file paths |
| `xl` | 160+ | Full: side-by-side, extra context |

Detection: `${COLUMNS:-$(tput cols 2>/dev/null || echo 80)}`

### V-model visualization

```text
# xs (67 cols) — compact
  V-DOWN          V-UP
  [x] Motivation  [ ] Acceptance
  [x] Options     [ ] System
  [x] Design      [>] Integration
  [x] Contract    [x] Unit
       [x] Build

# md (80+ cols) — standard
  V-DOWN                          V-UP
  ──────                          ──────
  [x] Motivation (ADR-121)        [ ] Acceptance (PR review)
  [x] Options (Option C)          [ ] System (CI green)
  [x] Design (modules split)      [>] Integration (make full-check)
  [x] Contract (cpm.toml)         [x] Unit (make quick-check)
              [x] Build (make build)
```

### Status symbols (no emoji, accessible)

| Status | Symbol | Color | NO_COLOR |
|--------|--------|-------|----------|
| Done | `[x]` | green | `[done]` |
| Active | `[>]` | cyan | `[active]` |
| Todo | `[ ]` | gray | `[todo]` |
| Blocked | `[!]` | red | `[blocked]` |

### Responsive text

| Element | xs | sm | md | lg |
|---------|----|----|----|----|
| Check name | 12 chars + ellipsis | 18 chars | full | full |
| Timing | hidden | seconds only | ms precision | ms + trend |
| File path | hidden | hidden | basename | full path |
| Trend | hidden | hidden | arrow only | arrow + percentage |

### Configuration

```toml
[ui]
mode = "auto"
breakpoints = { xs = 60, sm = 80, md = 120, lg = 160 }
# Override: CPM_COLUMNS=80 forces md layout
```

## Implementation

<!-- TODO: add _cpm_cols() to ui.sh that detects terminal width -->
<!-- TODO: update print_step to truncate based on available width -->
<!-- TODO: create v-model.sh that renders the V visualization -->
<!-- TODO: wire into make next -->

## References

- @see lib/cpm/shell/ui.sh (current TUI)
- @see docs/adr/adr-123-accessible-tui-output.md (accessibility)
- @see docs/adr/adr-121-cpm-quality-layer.md (V-model workflow)
