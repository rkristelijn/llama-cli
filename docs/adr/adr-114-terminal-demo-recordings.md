---
summary: Terminal Demo Recordings with VHS
status: accepted
---

# ADR-114: Terminal Demo Recordings with VHS

## Context

All roadmap features are implemented. The README needs to show what llama-cli can do rather than list what's planned. Static code blocks don't convey the streaming, interactive experience.

We need reproducible, version-controlled terminal demos that can be re-recorded after UI changes and embedded in the README or docs site.

## Decision

Use [VHS](https://github.com/charmbracelet/vhs) (Charmbracelet) to create animated terminal recordings from declarative `.tape` files stored in `demos/`.

### Why VHS

- Declarative: `.tape` files are plain text, diffable, reviewable
- Reproducible: same tape → same output (given same model responses)
- Multiple formats: GIF, MP4, WebM from one source
- No binary dependencies beyond ffmpeg/ttyd (auto-installed via Homebrew)
- Active maintenance by Charmbracelet (Bubble Tea, Lip Gloss ecosystem)

### Tape inventory

| Tape | Feature demonstrated |
|------|---------------------|
| `chat.tape` | Streaming chat, markdown rendering |
| `file-io.tape` | File read, write, str_replace |
| `agents.tape` | Agent switching, themes |
| `web-search.tape` | SearXNG web search |
| `vision.tape` | Image attachment |
| `smart-routing.tape` | Auto model selection |

### Recording workflow

```bash
brew install charmbracelet/tap/vhs
vhs demos/chat.tape              # single
for t in demos/*.tape; do vhs "$t"; done  # all
```

Generated GIFs are not committed (`.gitignore`); they are produced on demand or in CI.

## Consequences

### Positive

- README shows real capabilities instead of a roadmap checklist
- Demos stay in sync with code (re-record after changes)
- Low maintenance: tape files rarely need updating unless commands change
- Can be embedded in GitHub README, docs site, or blog posts

### Negative

- Recordings depend on Ollama running with a specific model
- LLM output is non-deterministic; recordings may vary between runs
- GIF file sizes can be large for long demos (mitigate with short tapes)
