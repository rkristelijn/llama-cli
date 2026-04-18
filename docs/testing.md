# Testing & Coverage Guide

## Test structure

Tests live next to the source they test:

| Pattern | Type | Example |
|---|---|---|
| `*_test.cpp` | Unit test | `config_test.cpp` |
| `*_it.cpp` | Integration test | `commands_it.cpp` |
| `*_test_helper.cpp` | Shared test stubs | `config_test_helper.cpp` |

All test binaries are named `test_*` and built by CMake. Both `make test` and `make coverage` auto-discover and run every `test_*` binary in the build directory — no hardcoded list.

## Running tests

```bash
make test              # build + run all tests
make e2e               # end-to-end tests (no Ollama needed)
make live              # integration test with real LLM
make coverage          # build with --coverage, run tests
make coverage-folder   # per-directory coverage summary
```

## Coverage measurement

### How it works

1. `make coverage` builds with `--coverage` flags and runs all `test_*` binaries
2. CI uses `lcov` to capture `.gcda` data and compute line coverage
3. Files are excluded via `lcov --remove` patterns

### Pitfalls we learned the hard way

**Pitfall 1: Hardcoded test lists go stale.**
When new test binaries are added to `CMakeLists.txt`, a hardcoded list in the coverage script won't pick them up. Both `run-coverage.sh` and `test-unit.sh` now glob `build/test_*` instead.

**Pitfall 2: lcov exclude filters must match actual paths.**
Dependencies fetched by CMake end up in `build/_deps/` with names like `httplib-src/`, `dtl-src/`, `linenoise-src/`. A filter like `*/httplib/*` does NOT match `*/httplib-src/httplib.h`. Always verify filters against `lcov --list coverage.info`.

Current CI exclude patterns:

```text
/usr/*              — system headers
*_test.cpp          — unit test files
*_it.cpp            — integration test files
*_test_helper.cpp   — test helper stubs
*/doctest/*         — test framework
*/httplib-src/*     — HTTP library
*/dtl-src/*         — diff library
*/linenoise-src/*   — line editing library
*/test_helpers.h    — shared test utilities
```

**Pitfall 3: Test files in `src/` are not in a `test/` directory.**
A filter like `*/test/*` won't exclude `src/config/config_test.cpp`. Use filename patterns (`*_test.cpp`) instead of directory patterns.

**Pitfall 4: Integration tests link against production code.**
Integration tests (`*_it.cpp`) link against `REPL_LIBS` which includes `repl.cpp`, `ollama.cpp`, `logger.cpp`, etc. Skipping them from coverage runs leaves large modules at 0%.

**Pitfall 5: Test helper stubs shadow real code.**
`test_config_it` links against `config_test_helper.cpp` (a minimal stub) instead of `config.cpp`. Coverage of `config.cpp` only comes from `test_config`. If the stub diverges from the real implementation, tests can segfault.

### Debugging coverage locally

```bash
# Full measurement (same as CI)
rm -rf build
make coverage
lcov --capture --directory build --output-file cov.info \
  --ignore-errors mismatch,unused
lcov --remove cov.info '/usr/*' '*_test.cpp' '*_it.cpp' \
  '*_test_helper.cpp' '*/doctest/*' '*/httplib-src/*' \
  '*/dtl-src/*' '*/linenoise-src/*' '*/test_helpers.h' \
  --output-file cov.info --ignore-errors unused

# Check what's included
lcov --list cov.info

# Summary
lcov --summary cov.info
```

## Adding a new test

1. Create `src/<module>/<name>_test.cpp` (unit) or `<name>_it.cpp` (integration)
2. Add `add_executable` + `target_include_directories` + `target_link_libraries` to `CMakeLists.txt`
3. Run `make test` — the new binary is auto-discovered
4. Run `make coverage-folder` to verify coverage improved
