# Demos

Terminal demo recordings using [VHS](https://github.com/charmbracelet/vhs) by Charmbracelet.

## Prerequisites

```bash
brew install charmbracelet/tap/vhs   # macOS
# or: go install github.com/charmbracelet/vhs@latest
```

VHS also requires `ffmpeg` and `ttyd` (installed automatically by Homebrew).

## Recording

```bash
# Record a single demo
vhs demos/chat.tape

# Record all demos
for tape in demos/*.tape; do vhs "$tape"; done
```

## Tapes

| Tape | Shows |
|------|-------|
| `chat.tape` | Basic chat, markdown rendering, code blocks |
| `file-io.tape` | File read, write, str_replace edits |
| `agents.tape` | Agent switching, themes |
| `web-search.tape` | SearXNG web search integration |
| `vision.tape` | Image attachment for multimodal models |
| `smart-routing.tape` | Auto model selection by complexity |

## Tips

- Ensure Ollama is running before recording
- For web-search demo, SearXNG must be running on port 8888
- Adjust `Sleep` durations if your model is faster/slower
- Output formats: `.gif`, `.mp4`, `.webm` (change the `Output` line)
