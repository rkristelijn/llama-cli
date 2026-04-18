# Prompt 05: Bump test coverage threshold from 55% to 60%

## Context

- CI coverage check is in `.github/workflows/ci.yml` in the `test-coverage` job
- The current threshold is 55% (line contains: `$PERCENT < 55`)
- Local coverage check is in `scripts/check/coverage.sh` with `THRESHOLD=80` (per-file)
- We need the CI global threshold to be 60% for CMMI 1 check 1.1

## Task

Change the CI coverage threshold from 55% to 60%.

## Change: Update `.github/workflows/ci.yml`

Find this exact line:

```yaml
          [ "$(echo "$PERCENT < 55" | bc -l)" -eq 0 ] || { echo "Coverage below 55%!"; exit 1; }
```

Replace it with:

```yaml
          [ "$(echo "$PERCENT < 60" | bc -l)" -eq 0 ] || { echo "Coverage below 60%!"; exit 1; }
```

## Verify

```bash
# 1. Check the threshold was updated
grep "PERCENT < 60" .github/workflows/ci.yml && echo "PASS: threshold is 60%" || echo "FAIL: threshold not updated"

# 2. Confirm old threshold is gone
grep "PERCENT < 55" .github/workflows/ci.yml && echo "FAIL: old threshold still present" || echo "PASS: old threshold removed"

# 3. Run local tests to check current coverage
make test && echo "PASS: tests pass" || echo "FAIL: tests broken"
```

## Expected output

```
PASS: threshold is 60%
PASS: old threshold removed
PASS: tests pass
```

## Note

If CI fails after this change because coverage is between 55-60%, you need to add more tests. Check which modules have low coverage by running `make coverage` locally.

## Commit message

```
chore: bump CI coverage threshold from 55% to 60%
```
