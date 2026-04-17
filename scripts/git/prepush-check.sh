#!/usr/bin/env bash
#
# prepush-check.sh — Run pre-push checks (skips lints done in pre-commit).
#
# If code changed: runs analysis, tests, security (lints already done).
# If only docs: verifies INDEX.md is up-to-date.
#
# Usage:
#   bash scripts/git/prepush-check.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

PASSED=0
FAILED=0
STEP=0
FAILED_NAMES=()

# Pre-push checks (lints already done in pre-commit)
STEPS=(
  "Analysis|tidy|make -s tidy"
  "Analysis|complexity|make -s complexity"
  "Analysis|docs|make -s docs"
  "Analysis|index|make -s index"
  "Test|coverage|make -s coverage-folder"
  "Security|sast-security|make -s sast-security"
  "Metrics|comment-ratio|bash scripts/check/comment-ratio.sh"
)

TOTAL="${#STEPS[@]}"

declare -A HINTS=(
  ["tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["complexity"]="fix: refactor or add pmccabe:skip-complexity, recheck: make complexity"
  ["docs"]="fix: address doxygen warnings in source, recheck: make docs"
  ["index"]="fix: make index, recheck: make index"
  ["coverage"]="fix: add tests, recheck: make coverage-folder"
  ["sast-security"]="fix: address semgrep findings, recheck: make sast-security"
  ["comment-ratio"]="fix: add comments to source, recheck: make comment-ratio"
)

run_step() {
  local phase="$1" name="$2" cmd="$3"
  (( STEP++ )) || true
  local output start elapsed
  printf "  [%d/%d] %s... " "${STEP}" "${TOTAL}" "${name}"
  start="$(date +%s)"
  if output="$(eval "${cmd}" 2>&1)"; then
    elapsed="$(( $(date +%s) - start ))"
    printf "✓ (%ds)\n" "${elapsed}"
    (( PASSED++ )) || true
  else
    elapsed="$(( $(date +%s) - start ))"
    printf "✗ (%ds)\n" "${elapsed}"
    echo "${output}" | sed 's/^/    /'
    FAILED_NAMES+=("${name}")
    (( FAILED++ )) || true
  fi
}

main() {
  local changed
  changed="$(git diff --name-only origin/main...HEAD)"

  if ! echo "${changed}" | grep -qE '\.(cpp|h)$'; then
    echo "==> make index (docs only)"
    make -s index
    git diff --quiet INDEX.md || { echo "FAIL: INDEX.md outdated"; exit 1; }
    echo "All checks passed."
    return 0
  fi

  local current_phase=""
  for entry in "${STEPS[@]}"; do
    local phase="${entry%%|*}"
    local rest="${entry#*|}"
    local name="${rest%%|*}"
    local cmd="${rest#*|}"

    if [[ "${phase}" != "${current_phase}" ]]; then
      echo ""
      echo "── ${phase} ──"
      current_phase="${phase}"
    fi

    run_step "${phase}" "${name}" "${cmd}"
  done

  echo ""
  if (( FAILED > 0 )); then
    echo "Failed:"
    for name in "${FAILED_NAMES[@]}"; do
      local hint="${HINTS[${name}]:-run: make ${name}}"
      echo "  ✗ ${name} — ${hint}"
    done
    echo ""
    echo "${PASSED} passed, ${FAILED} failed."
    exit 1
  fi
  echo "All ${TOTAL} checks passed."
}

main "$@"
