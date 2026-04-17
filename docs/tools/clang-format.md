# clang-format

| | |
|---|---|
| Version | 19 (LLVM) (pinned in `.config/versions.env`) |
| Purpose | Automatic C++ code formatter |
| Config | `.config/.clang-format` |
| Make target | `make format` / `make format-check` |
| Official docs | <https://clang.llvm.org/docs/ClangFormat.html> |

## Usage

```bash
make format          # format all C++ files in-place
make format-check    # check formatting without modifying files
```

## Notes

- `make format-check` is used in CI to enforce consistent style
- Config is symlinked or referenced from `.config/.clang-format`
