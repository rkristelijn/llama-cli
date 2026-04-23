#!/usr/bin/env bash
#
# run-coverage.sh — Build with coverage flags and run all tests.
#
# Usage:
#   bash scripts/check/run-coverage.sh [build_dir]
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"

main() {
  echo "==> make coverage (configuring with --coverage...)"
  cmake -B "${BUILD_DIR}" -S . -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" -DENABLE_FUZZ=OFF > /dev/null
  cmake --build "${BUILD_DIR}" > /dev/null
  echo "==> make coverage (running tests...)"
  for t in "${BUILD_DIR}"/test_*; do
    [ -x "$t" ] || continue
    "$t" --quiet
  done
  echo "  [done] coverage"
}

main "$@"
