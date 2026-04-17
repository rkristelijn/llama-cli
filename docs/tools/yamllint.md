# yamllint

| | |
|---|---|
| Version | 1.33.0 (pinned in `.config/versions.env`) |
| Purpose | YAML file linter for syntax and style |
| Config | `.config/yamllint.yml` |
| Make target | `make yamllint` |
| Official docs | <https://yamllint.readthedocs.io/> |

## Usage

```bash
make yamllint
```

## Notes

- Lints all `.yml` and `.yaml` files in the project
- Config in `.config/yamllint.yml` defines rules for line length, indentation, etc.
