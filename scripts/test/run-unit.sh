#!/usr/bin/env bash
#
# test-unit.sh — Build and run all unit tests.
#
# Usage:
#   bash scripts/check/test-unit.sh [build_dir]
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"

main() {
  echo "==> make test-unit"
  if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "  [error] No build found in ${BUILD_DIR}/. Run 'make build' first." >&2
    exit 1
  fi
  for t in "${BUILD_DIR}"/test_*; do
    [ -x "$t" ] || continue
    name="$(basename "$t")"
    if ! cmake --build "${BUILD_DIR}" --target "${name}" >/dev/null 2>&1; then
      echo "  [error] Failed to build ${name}. Run 'make build' to see errors." >&2
      exit 1
    fi
  done
  # Remove stale coverage data after rebuild to prevent gcda merge errors
  find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true
  local total=0 passed=0 failed=0
  for t in "${BUILD_DIR}"/test_*; do
    [ -x "$t" ] || continue
    local name
    name="$(basename "$t")"
    local count
    count=$("$t" --list-test-cases 2>/dev/null | wc -l)
    if "$t" --no-version --quiet 2>/dev/null; then
      passed=$((passed + count))
    else
      echo "  ✗ ${name} FAILED"
      failed=$((failed + count))
    fi
    total=$((total + count))
  done
  if [[ "$failed" -gt 0 ]]; then
    echo "  FAIL: ${passed}/${total} passed, ${failed} failed"
    exit 1
  fi
  echo "  ✓ ${total} scenarios across $(ls "${BUILD_DIR}"/test_* 2>/dev/null | wc -l) binaries"
  echo "  [done] test-unit"
}

main "$@"
