#!/usr/bin/env bash
#
# quick.sh — Run unit tests and comment ratio check (fast feedback loop).
#
# Usage:
#   bash scripts/dev/quick.sh [build_dir]
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"
TESTS=(test_config test_json test_repl test_command test_annotation test_exec)

main() {
  for t in "${TESTS[@]}"; do
    ./"${BUILD_DIR}/${t}" --quiet
  done
  bash scripts/lint/check-comment-ratio.sh | grep "PASS" || bash scripts/lint/check-comment-ratio.sh
}

main "$@"
