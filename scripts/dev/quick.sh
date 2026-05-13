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

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
BUILD_DIR="${1:-build}"
TESTS=(test_config test_json test_repl test_command test_annotation test_exec)

main() {
  for t in "${TESTS[@]}"; do
    ./"${BUILD_DIR}/${t}" --quiet
  done
  bash scripts/lint/check-comment-ratio.sh | grep "PASS" || bash scripts/lint/check-comment-ratio.sh
}

main "$@"
