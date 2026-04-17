# Semgrep

| | |
|---|---|
| Version | 1.56.0 (pinned in `.config/versions.env`) |
| Purpose | SAST security scanner for finding vulnerabilities |
| Config | none |
| Make target | `make sast-security` |
| Official docs | <https://semgrep.dev/docs/> |

## Usage

```bash
make sast-security
```

## Notes

- Uses Semgrep's built-in rulesets; no project-level config file needed
- Runs in CI to catch security issues before merge
