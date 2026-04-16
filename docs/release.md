# Release Process

## Version Bump

1. Update `VERSION` file in root (semver format, e.g., `0.17.0`)
2. Commit the change
3. Create a tag with `v` prefix:

```bash
git add VERSION
git commit -m "Bump version to X.Y.Z"
git tag vX.Y.Z
git push origin main --tags
```

## What Happens

The [release workflow](../.github/workflows/release.yml) automatically:
- Builds the binary on Ubuntu
- Creates a GitHub Release from the tag
- Uploads the `llama-cli` binary as a release asset

## Download

Users download from: https://github.com/ollama/llama-cli/releases