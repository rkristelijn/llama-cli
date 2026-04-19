#!/usr/bin/env bash
# DEPRECATED: use 'make check' instead. See ADR-044.
echo "DEPRECATED: scripts/check/run-all.sh — use 'make check' instead" >&2; exit 1
#
# Usage:
#   bash scripts/check/run-all.sh
#   FULL=1 bash scripts/check/run-all.sh
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

# Phase definitions: "phase|name|command"
STEPS=(
  "Lint|lint-code|make -s lint-code"
  "Lint|lint-yaml|make -s lint-yaml"
  "Lint|lint-markdown|make -s lint-markdown"
  "Lint|lint-makefile|make -s lint-makefile"
  "Lint|lint-scripts|make -s lint-scripts"
  "Analysis|tidy|make -s tidy"
  "Analysis|complexity|make -s complexity"
  "Analysis|docs|make -s docs"
  "Analysis|index|make -s index"
  "Test|coverage|make -s coverage-folder"
  "Security|sast-security|make -s sast-security"
  "Security|sast-secret|make -s sast-secret"
  "Metrics|comment-ratio|bash scripts/check/comment-ratio.sh"
)

TOTAL="${#STEPS[@]}"

# Autofix hints per step name
declare -A HINTS=(
  ["lint-code"]="fix: make format-code, recheck: make lint-code"
  ["lint-yaml"]="fix: edit .github/*.yml, recheck: make lint-yaml"
  ["lint-markdown"]="fix: rumdl fmt ., recheck: make lint-markdown"
  ["lint-makefile"]="fix: extract target to scripts/, recheck: make lint-makefile"
  ["lint-scripts"]="fix: see docs/tools/shell-scripts.md, recheck: make lint-scripts"
  ["tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["complexity"]="fix: refactor or add pmccabe:skip-complexity, recheck: make complexity"
  ["docs"]="fix: address doxygen warnings in source, recheck: make docs"
  ["index"]="fix: make index, recheck: make index"
  ["coverage"]="fix: add tests, recheck: make coverage-folder"
  ["sast-security"]="fix: address semgrep findings, recheck: make sast-security"
  ["sast-secret"]="fix: remove secrets from code, recheck: make sast-secret"
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
