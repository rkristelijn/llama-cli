# Doxygen

| | |
|---|---|
| Version | 1.16.1 (pinned in `.config/versions.env`) |
| Purpose | API documentation generator from annotated C++ source |
| Config | `.config/Doxyfile` |
| Make target | `make docs` |
| Official docs | <https://www.doxygen.nl/> |

## Usage

```bash
make docs
```

## Notes

- Generated documentation is output to `docs/html/` (git-ignored)
- Doxyfile in `.config/Doxyfile` controls input paths and output format
