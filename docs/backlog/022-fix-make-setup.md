# 022: Fix `make setup` — Missing Tools

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#86](https://github.com/rkristelijn/llama-cli/issues/86)

## Problem

`make setup` reports missing tools but doesn't install them:

```text
clang-tidy missing
pmccabe missing
```

## Fix

### clang-tidy (macOS)

No standalone brew formula exists. Must install full LLVM and symlink:

```bash
brew install llvm
ln -s "$(brew --prefix llvm)/bin/clang-tidy" "/usr/local/bin/clang-tidy"
```

Note: `brew install llvm` is ~2.8GB. The `fmenezes/tap/clang-tidy` tap is dead (confirmed 2023+).

Source: <https://stackoverflow.com/questions/53111082/how-to-install-clang-tidy-on-macos>

### pmccabe (macOS)

```bash
brew install pmccabe
```

### What to change in setup.sh

- Auto-install missing tools via `brew install` (macOS) / `apt install` (Linux)
- Or at minimum print the install command so the user can copy-paste
- `make setup` should exit non-zero if required tools are missing and not auto-installed

## References

- [ADR-011: Self-Contained Setup](../adr/adr-011-self-contained-setup.md)
