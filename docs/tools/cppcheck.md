# cppcheck

| | |
|---|---|
| Version | 2.13 (pinned in `.config/versions.env`) |
| Purpose | C++ static analysis for bugs and undefined behavior |
| Config | none |
| Make target | `make lint` |
| Official docs | <https://cppcheck.sourceforge.io/> |

## Usage

```bash
make lint
```

## Notes

- Runs without a project config file; all options are set in the Makefile
- Complements clang-tidy by catching different classes of defects
