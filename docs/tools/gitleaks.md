# Gitleaks

| | |
|---|---|
| Version | 8.18.2 (pinned in `.config/versions.env`) |
| Purpose | Secret scanner to prevent credentials from being committed |
| Config | none |
| Make target | `make sast-secret` |
| Official docs | <https://gitleaks.io/> |

## Usage

```bash
make sast-secret
```

## Notes

- Scans the full git history for leaked secrets, tokens, and keys
- Runs in CI alongside Semgrep as part of the SAST pipeline
