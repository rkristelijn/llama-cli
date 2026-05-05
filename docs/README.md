# Documentation

Llama CLI is a C++ TUI that connects to a local [Ollama](https://ollama.com) instance for interactive LLM conversations.

## Guides

- [User Guide](user-guide.md) — installation, usage, configuration
- [Task Status](task-status.md) — checkbox convention: `[ ]` `[-]` `[>]` `[~]` `[x]` `[/]`
- [Clang-Tidy Guide](clang-tidy.md) — static analysis, best practices, and suppressions
- [Architecture](architecture.md) — how it works internally
- [Architecture V2](architecture-v2.md) — multi-model provider system
- [Ollama Setup](ollama-setup.md) — install and configure Ollama
- [Multi-Model Guide](multi-model-guide.md) — Ollama & Gemini integration
- [Provider Roadmap](provider-roadmap.md) — implementation plan for multi-model providers
- [Testing](testing.md) — testing & coverage guide
- [Full Test Script](full-test-script.md) — manual test script
- [Model Benchmark](model-bench.md) — model comparison results
- [PR Workflow](pr-workflow.md) — pull request & release workflow
- [Release](release.md) — release process
- [Credits in AI](credits-in-ai.md) — using AI efficiently
- [Make Commands](make.md) — make command tree
- [GitHub Integration](github-integration.md) — working with GitHub from CLI
- [GitHub CLI Manual](gh-manual.md) — gh command reference
- [CodeRabbit](code-rabbit.md) — CodeRabbit PR review integration

## Tools

All tools are pinned in [`.config/versions.env`](../.config/versions.env). Run `make check-versions` to verify.

| Tool | Purpose | Make target | Docs |
|------|---------|-------------|------|
| [clang-format](tools/clang-format.md) | C++ formatting | `make format-check` | [llvm.org](https://clang.llvm.org/docs/ClangFormat.html) |
| [clang-tidy](tools/clang-tidy.md) | C++ linting | `make tidy` | [llvm.org](https://clang.llvm.org/extra/clang-tidy/) |
| [cppcheck](tools/cppcheck.md) | C++ static analysis | `make lint` | [cppcheck.net](https://cppcheck.sourceforge.io/) |
| [pmccabe](tools/pmccabe.md) | Cyclomatic complexity | `make complexity` | [manpage](https://manpages.ubuntu.com/manpages/noble/man1/pmccabe.1.html) |
| [doxygen](tools/doxygen.md) | Documentation generation | `make docs` | [doxygen.nl](https://www.doxygen.nl/) |
| [semgrep](tools/semgrep.md) | SAST security scan | `make sast-security` | [semgrep.dev](https://semgrep.dev/) |
| [gitleaks](tools/gitleaks.md) | Secret scanning | `make sast-secret` | [github.com](https://github.com/gitleaks/gitleaks) |
| [shellcheck](tools/shellcheck.md) | Shell script linting | `make shellcheck` | [shellcheck.net](https://www.shellcheck.net/) |
| [yamllint](tools/yamllint.md) | YAML linting | `make yamllint` | [github.com](https://github.com/adrienverge/yamllint) |
| [rumdl](tools/rumdl.md) | Markdown linting | `make markdownlint` | [rumdl.dev](https://rumdl.dev/) |
| [Shell conventions](tools/shell-scripts.md) | Script standards | `make lint-scripts` | — |

## Architecture Decision Records

See [adr/](adr/README.md) for all decisions and their rationale.

## Roadmap

See [roadmap/](roadmap/README.md) for the phased plan toward daily-driver status.
