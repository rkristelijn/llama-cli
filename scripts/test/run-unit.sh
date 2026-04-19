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
  for t in "${BUILD_DIR}"/test_*; do
    [ -x "$t" ] || continue
    name="$(basename "$t")"
    cmake --build "${BUILD_DIR}" --target "${name}" > /dev/null
  done
  for t in "${BUILD_DIR}"/test_*; do
    [ -x "$t" ] || continue
    "$t" --quiet
  done
  echo "  [done] test-unit"
}

main "$@"
