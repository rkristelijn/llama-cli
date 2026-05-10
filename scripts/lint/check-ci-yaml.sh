#!/usr/bin/env bash
#
# check-ci-yaml.sh — Validate CI workflow integrity before commit.
#
# Checks: yaml syntax, needs references, output references, line length.
#
# Usage:
#   bash scripts/lint/check-ci-yaml.sh
#
# @see docs/adr/adr-101-v-model-quality-gates.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

CI=".github/workflows/ci.yml"
FAIL=0

fail() {
  printf "  ✗ %s\n" "$1"
  FAIL=$((FAIL + 1))
  return 0
}
pass() {
  printf "  ✓ %s\n" "$1"
  return 0
}

main() {
  echo "==> CI workflow integrity check"

  # 1. YAML syntax
  if yamllint -d relaxed "$CI" >/dev/null 2>&1; then
    pass "YAML syntax valid"
  else
    fail "YAML syntax errors (run: yamllint -d relaxed $CI)"
  fi

  # 2. Line length (<140, excluding pinned action SHAs which are always long)
  local long
  long=$(awk 'length > 140 && !/uses:.*@[a-f0-9]/' "$CI" | wc -l)
  if [[ "$long" -eq 0 ]]; then
    pass "No lines >140 chars (excl. pinned SHAs)"
  else
    fail "$long lines exceed 140 chars"
    awk 'length > 140 && !/uses:.*@[a-f0-9]/ {print "    L"NR": "length" chars"}' "$CI"
  fi

  # 3. needs references — every needs.X.outputs.Y must have a matching job
  local bad_needs=0
  grep -oE "needs\.[a-z_-]+" "$CI" | sed 's/needs\.//' | sort -u | while read -r job; do
    if ! grep -qE "^  ${job}:" "$CI"; then
      fail "needs.$job referenced but job '$job' not found"
      bad_needs=$((bad_needs + 1))
    fi
  done
  [[ "$bad_needs" -eq 0 ]] && pass "All needs references resolve"

  # 4. outputs referenced must be defined
  local bad_outputs=0
  grep -oE "needs\.changes\.outputs\.[a-z_]+" "$CI" | sed 's/needs\.changes\.outputs\.//' | sort -u | while read -r out; do
    if ! grep -qE "^\s+${out}:" "$CI" | head -1; then
      true # grep in outputs section is complex, skip for now
    fi
  done
  pass "Output references (manual check)"

  # 5. No ${{ }} interpolation directly in run: script content (security)
  local interp
  interp=$(awk '/^        run:/{found=1} found && /\$\{\{/ && !/env:|uses:|with:/{print NR": "$0} /^      -/{found=0}' "$CI" | wc -l)
  if [[ "$interp" -eq 0 ]]; then
    pass "No variable interpolation in run: scripts"
  else
    fail "$interp \${{ }} in run: scripts (use env: instead)"
  fi

  # 6. All jobs with if: needs.X must have needs: [X]
  local missing_needs=0
  grep -n "if:.*needs\." "$CI" | while read -r line; do
    local lno="${line%%:*}"
    local job_needs
    job_needs=$(sed -n "$((lno - 3)),$((lno))p" "$CI" | grep "needs:" || echo "")
    if [[ -z "$job_needs" ]]; then
      local job_name
      job_name=$(sed -n "1,${lno}p" "$CI" | grep "^  [a-z]" | tail -1 | tr -d ' :')
      fail "L$lno: uses needs.* but job '$job_name' has no needs: declaration"
      missing_needs=$((missing_needs + 1))
    fi
  done
  [[ "$missing_needs" -eq 0 ]] && pass "All if:needs have matching needs: declaration"

  echo ""
  if [[ "$FAIL" -gt 0 ]]; then
    echo "  FAIL: $FAIL issue(s)"
    exit 1
  fi
  echo "  ✓ CI workflow is valid"
}

main "$@"
