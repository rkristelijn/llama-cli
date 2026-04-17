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
TESTS=(test_config test_json test_repl test_command test_annotation test_exec)

main() {
  echo "==> make test-unit"
  for t in "${TESTS[@]}"; do
    cmake --build "${BUILD_DIR}" --target "${t}" > /dev/null
  done
  for t in "${TESTS[@]}"; do
    ./"${BUILD_DIR}/${t}" --quiet
  done
  echo "  [done] test-unit"
}

main "$@"
