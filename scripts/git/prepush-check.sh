#!/usr/bin/env bash
#
# prepush-check.sh — Validate formatting + analysis + tests + security (13 checks).
#
# Formatting auto-fixed in pre-commit; this validates it stuck.
# sast-secret already runs in pre-commit, skipped here.
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

STEPS=(
  "Lint|cpp-format|make -s cpp-format"
  "Lint|yamllint|make -s yamllint"
  "Lint|markdownlint|make -s markdownlint"
  "Lint|lint-makefile|make -s lint-makefile"
  "Lint|lint-scripts|make -s lint-scripts"
  "Analysis|tidy|make -s tidy"
  "Analysis|complexity|make -s complexity"
  "Analysis|lint|make -s lint"
  "Analysis|docs|make -s docs"
  "Analysis|index|make -s index"
  "Test|coverage|make -s coverage-folder"
  "Security|sast-security|make -s sast-security"
  "Metrics|comment-ratio|bash scripts/check/comment-ratio.sh"
)

TOTAL="${#STEPS[@]}"

declare -A HINTS=(
  ["cpp-format"]="fix: make format-cpp, recheck: make cpp-format"
  ["yamllint"]="fix: make format-yaml, recheck: make yamllint"
  ["markdownlint"]="fix: make format-markdown, recheck: make markdownlint"
  ["lint-makefile"]="fix: extract target to scripts/, recheck: make lint-makefile"
  ["lint-scripts"]="fix: make format-scripts, recheck: make lint-scripts"
  ["tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["complexity"]="fix: refactor or add pmccabe:skip-complexity, recheck: make complexity"
  ["lint"]="fix: address cppcheck warnings, recheck: make lint"
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
    echo "==> docs only — verifying INDEX.md"
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
