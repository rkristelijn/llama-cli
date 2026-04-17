# ADR-011: Self-Contained Setup

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: The project depends on 7 external tools. New developers should not have to figure out what to install manually. The repo must be portable and self-contained.

## Decision

A `make setup` target is provided that detects the platform (macOS/Linux) and installs all missing dependencies automatically.

### Dependencies

| Tool | Purpose |
|------|---------|
| cmake | Build system |
| clang-format | Code formatting |
| cppcheck | Static analysis |
| semgrep | Security scanning |
| gitleaks | Secret detection |
| cloc | Comment ratio |
| ollama | LLM runtime |

### Onboarding

```bash
git clone https://github.com/rkristelijn/llama-cli
cd llama-cli
make setup         # installs everything + git hooks
sudo make install  # build and install to /usr/local/bin
llama-cli --help   # verify
```

## Rationale

- A new developer should go from clone to working in under 5 minutes
- Platform detection (brew/apt) makes the repo portable across macOS and Linux
- The setup script is self-documenting — reading it shows all dependencies
- Git hooks are installed automatically so quality gates are enforced from the first commit

## Consequences

- `scripts/dev/setup.sh` must be maintained when new tools are added
- Only macOS (brew) and Linux (apt) are supported
- Windows is not supported (WSL can be used)
