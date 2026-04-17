# clang-tidy

| | |
|---|---|
| Version | 19 (LLVM) (pinned in `.config/versions.env`) |
| Purpose | C++ static analysis and linting |
| Config | `.config/.clang-tidy` |
| Make target | `make tidy` |
| Official docs | <https://clang.llvm.org/extra/clang-tidy/> |

## Usage

```bash
make tidy
```

## Notes

- Checks for common C++ bugs, style violations, and modernization opportunities
- Config in `.config/.clang-tidy` defines enabled check categories
