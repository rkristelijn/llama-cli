#!/usr/bin/env bash
# precommit-check.sh — Auto-fix formatting + secret scan (6 checks).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

STEP=0
TOTAL=6

run_step() {
  local name="$1"; shift
  (( STEP++ )) || true
  printf "  [%d/%d] %s... " "${STEP}" "${TOTAL}" "${name}"
  if output=$("$@" 2>&1); then
    printf "✓\n"
  else
    printf "✗\n"
    printf '%s\n' "${output}" | sed 's/^/    /'
    exit 1
  fi
}

echo ""
echo "── Formatting ──"
run_step "format-code" make -s format-code
run_step "format-yaml" make -s format-yaml
run_step "format-md" make -s format-md
run_step "format-scripts" make -s format-scripts

echo ""
echo "── Documentation ──"
run_step "index" make -s index

echo ""
echo "── Security ──"
run_step "sast-secret" make -s sast-secret

echo ""
echo "All ${TOTAL} checks passed."
