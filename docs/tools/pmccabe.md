# pmccabe

| | |
|---|---|
| Version | 2.8 (pinned in `.config/versions.env`) |
| Purpose | Cyclomatic complexity measurement for C/C++ |
| Config | none |
| Make target | `make complexity` |
| Official docs | <https://people.debian.org/~bame/pmccabe/> |

## Usage

```bash
make complexity
```

## Notes

- Reports per-function cyclomatic complexity to identify overly complex code
- No config file; thresholds are defined in the Makefile
