# ADR-070: Pluggable Mermaid Diagram Renderers

*Status*: Implemented

## Date

2026-05-02

## Context

The mermaid renderer (ADR-052) only supported `graph`/`flowchart` diagrams via braille art. When the LLM generated other diagram types (sequenceDiagram, pie, stateDiagram), the renderer either failed silently or left broken output artifacts (opening fence printed, content dumped raw, closing fence missing).

LLMs frequently generate sequence diagrams and pie charts. We need:

- A pluggable architecture so new diagram types can be added independently
- Graceful fallback for unsupported types (show as code block, not broken output)
- The system prompt to tell the model which types are visually rendered

## Decision

### Architecture: Strategy pattern with registry

```
DiagramRenderer (interface)
├── FlowchartRenderer  — braille art (existing engine)
├── SequenceRenderer   — ASCII columns with arrows
├── PieRenderer        — horizontal bar chart
└── StateRenderer      — converts to flowchart, delegates to braille
```

- `DiagramRegistry` holds all renderers, dispatches via `can_render()` (first match wins)
- Global `diagram_registry()` is lazily initialized with all built-in renderers
- `markdown.cpp` calls `diagram_registry().render()` instead of `render_mermaid()` directly
- On failure (no renderer matches or parse error), the mermaid block is shown as a normal code block

### Supported diagram types

| Type | Keyword | Rendering |
|------|---------|-----------|
| Flowchart | `graph TD/LR`, `flowchart` | Braille art (existing) |
| Sequence | `sequenceDiagram` | ASCII: participant columns + labeled arrows |
| Pie chart | `pie` | Horizontal bar chart with █/░ and percentages |
| State | `stateDiagram-v2` | Converted to flowchart → braille art |

### Fallback behavior

Unsupported types (gantt, classDiagram, erDiagram, etc.) are rendered as a normal fenced code block with the `mermaid` language tag — no broken output.

### File layout

```
src/tui/mermaid/
├── renderer.h       — DiagramRenderer interface + DiagramRegistry
├── renderer.cpp     — Registry implementation + global instance
├── flowchart.h/cpp  — FlowchartRenderer (delegates to mermaid.cpp)
├── sequence.h/cpp   — SequenceRenderer
├── pie.h/cpp        — PieRenderer
├── state.h/cpp      — StateRenderer
├── mermaid.h/cpp    — Original braille engine (unchanged)
└── renderer_test.cpp — Unit tests for all renderers
```

## Consequences

- Adding a new diagram type = one new class + register in `renderer.cpp`
- System prompt tells the model which types render visually
- No more broken output for unsupported diagram types
- Existing flowchart rendering is unchanged (wrapped, not modified)
