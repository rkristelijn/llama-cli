# ADR-032: E2E Test Improvements

*Status*: Accepted · *Date*: 2026-04-13 · *Context*: E2E tests had code duplication, weak assertions, and missing cleanup.

## Decision

Refactor e2e tests with shared helpers, stronger assertions, and proper cleanup.

### Changes

1. **Shared helpers** (`e2e/helpers.sh`):
   - `check_binary()` - binary existence check
   - `assert_nonempty()` - non-empty output validation
   - `assert_contains()` - substring validation
   - `die()` - consistent error handling
   - `setup_cleanup()` - trap helper for temp files

2. **Table-driven tests** (`test_sync_mode.sh`):
   - Multiple prompts with expected substrings
   - Scales without code duplication

3. **Cleanup trap** (`test_files_flag.sh`):
   - `trap 'rm -f "$TEMP_FILE"' EXIT`
   - Guarantees temp file removal even on failure

## Rationale

| Issue | Solution |
|-------|----------|
| Code duplication | Shared `helpers.sh` |
| Weak assertions | `assert_contains` with expected substrings |
| Missing cleanup | `trap` on EXIT |
| Brittle prompts | Table-driven approach |

## Consequences

- **Pros**: Consistent error handling, maintainable, better coverage
- **Cons**: Requires sourcing `helpers.sh` in each test

## Files Changed

- `e2e/helpers.sh` (new)
- `e2e/test_sync_mode.sh` (refactored)
- `e2e/test_repl_mode.sh` (refactored)
- `e2e/test_files_flag.sh` (refactored)
