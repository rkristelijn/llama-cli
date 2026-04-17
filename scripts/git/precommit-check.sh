#!/usr/bin/env bash
#
# precommit-check.sh — Fast pre-commit checks: build + secret scan.
#
# Usage:
#   bash scripts/git/precommit-check.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail

STEP=0
TOTAL=2

run_step() {
  local name="$1" cmd="$2"
  (( STEP++ )) || true
  local output start elapsed
  printf "  [%d/%d] %s... " "${STEP}" "${TOTAL}" "${name}"
  start="$(date +%s)"
  if output="$(eval "${cmd}" 2>&1)"; then
    elapsed="$(( $(date +%s) - start ))"
    printf "✓ (%ds)\n" "${elapsed}"
  else
    elapsed="$(( $(date +%s) - start ))"
    printf "✗ (%ds)\n" "${elapsed}"
    echo "${output}" | sed 's/^/    /'
    exit 1
  fi
}

echo "── Pre-commit checks ──"
run_step "format" "make format"
run_step "sast-secret" "make -s sast-secret"
echo ""
echo "All ${TOTAL} checks passed."
