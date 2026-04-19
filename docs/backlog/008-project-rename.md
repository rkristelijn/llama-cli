# 008: Project Rename

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#28](https://github.com/rkristelijn/llama-cli/issues/28)

## Problem

"llama-cli" conflicts with LLaMA (Meta), llama.cpp, and Ollama. The tool is model-agnostic — it can work with any LLM backend — so the name is misleading.

## Idea

Pick a model-agnostic name. Rename across: binary, repo, docs, config paths, env vars.

### Considerations

- Must not conflict with existing CLI tools
- Should convey "local AI terminal assistant"
- All `LLAMA_*` env vars and `~/.llama-cli/` paths need migration
- Backward compat: symlink old binary name for one release cycle
