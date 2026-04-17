# ShellCheck

| | |
|---|---|
| Version | 0.10.0 (pinned in `.config/versions.env`) |
| Purpose | Static analysis and linting for shell scripts |
| Config | none (uses `.shellcheckrc` when created) |
| Make target | `make shellcheck` (planned) |
| Official docs | <https://www.shellcheck.net/> |

## Usage

```bash
make shellcheck
```

## Notes

- Make target is planned but not yet implemented
- Will automatically pick up `.shellcheckrc` in the project root if present
