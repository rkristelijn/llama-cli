#!/usr/bin/env bash
#
# precommit-check.sh — Auto-fix formatting + secret scan (5 checks).
#
# Usage:
#   bash scripts/git/precommit-check.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail

STEP=0
TOTAL=6

run_step() {
  local name="$1"; shift
  (( STEP++ )) || true
  local output start elapsed
  printf "  [%d/%d] %s... " "${STEP}" "${TOTAL}" "${name}"
  start="$(date +%s)"
  if output="$("$@" 2>&1)"; then
    elapsed="$(( $(date +%s) - start ))"
    printf "✓ (%ds)\n" "${elapsed}"
  else
    elapsed="$(( $(date +%s) - start ))"
    printf "✗ (%ds)\n" "${elapsed}"
    printf '%s\n' "${output}" | sed 's/^/    /'
    exit 1
  fi
}

echo ""
echo "── Format ──"
run_step "format-code" make -s format-code
run_step "format-yaml" make -s format-yaml
run_step "format-markdown" make -s format-markdown
run_step "format-scripts" make -s format-scripts
echo ""
echo "── Docs ──"
run_step "index" make -s index
echo ""
echo "── Security ──"
run_step "sast-secret" make -s sast-secret
echo ""
echo "All ${TOTAL} checks passed."
