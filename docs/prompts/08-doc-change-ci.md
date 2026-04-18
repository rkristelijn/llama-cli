# Prompt 08: Add doc-change enforcement CI job

## Context

- CI config is at `.github/workflows/ci.yml`
- CMMI 1 check 1.8 requires docs to be updated when user-facing code changes
- The CI already has a `changes` job that detects code changes using `dorny/paths-filter`
- We need a new job that warns (not blocks) when `src/` changes but `docs/` doesn't

## Task

Add a `doc-check` job to `.github/workflows/ci.yml`.

## Change: Update `.github/workflows/ci.yml`

Find this exact text:

```yaml
  # --- Metrics ---

  comment-ratio:
```

Insert this new job BEFORE that line:

```yaml
  # --- Documentation ---

  doc-check:
    runs-on: ubuntu-24.04
    if: github.event_name == 'pull_request'
    steps:
      - uses: actions/checkout@34e114876b0b11c390a56381ad16ebd13914f8d5 # v4.3.1
        with:
          fetch-depth: 0
      - name: Check docs updated with code changes
        # @see docs/adr/adr-048-quality-framework.md §3.3 check 1.8
        run: |
          BASE="${{ github.event.pull_request.base.sha }}"
          HEAD="${{ github.event.pull_request.head.sha }}"
          SRC_CHANGED=$(git diff --name-only "$BASE" "$HEAD" -- 'src/' | wc -l | tr -d ' ')
          DOC_CHANGED=$(git diff --name-only "$BASE" "$HEAD" -- 'docs/' 'README.md' 'CHANGELOG.md' | wc -l | tr -d ' ')
          if [ "$SRC_CHANGED" -gt 0 ] && [ "$DOC_CHANGED" -eq 0 ]; then
            echo "::warning::Source code changed but no documentation was updated. Consider updating docs/ or CHANGELOG.md."
          else
            echo "Documentation check passed."
          fi

```

## Verify

```bash
# 1. Check the new job exists in CI config
grep -c "doc-check:" .github/workflows/ci.yml && echo "PASS: doc-check job exists" || echo "FAIL: job not found"

# 2. Check it references the ADR
grep -c "adr-048" .github/workflows/ci.yml && echo "PASS: ADR reference found" || echo "FAIL: no ADR reference"

# 3. Validate YAML syntax
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml'))" && echo "PASS: valid YAML" || echo "FAIL: invalid YAML"
```

## Expected output

```
PASS: doc-check job exists
PASS: ADR reference found
PASS: valid YAML
```

## Commit message

```
ci: add doc-change enforcement check
```
