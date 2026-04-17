#!/usr/bin/env bash
#
# prepush-check.sh — Validate all checks before pushing (15 checks).
#
# Names match CI workflow jobs exactly.
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
  "Lint|lint-cpp|make -s cpp-format"
  "Lint|lint-yaml|make -s yamllint"
  "Lint|lint-markdown|make -s markdownlint"
  "Lint|lint-makefile|make -s lint-makefile"
  "Lint|lint-scripts|make -s lint-scripts"
  "Lint|lint-tidy|make -s tidy"
  "Lint|lint-cppcheck|make -s lint"
  "Lint|lint-docs|make -s docs"
  "Lint|lint-complexity|make -s complexity"
  "Build|build|make -s build"
  "Test|unit-test|make -s test"
  "Test|e2e-test|make -s e2e"
  "Test|test-coverage|make -s coverage-folder"
  "Security|sast-security|make -s sast-security"
  "Metrics|comment-ratio|bash scripts/check/comment-ratio.sh"
)

TOTAL="${#STEPS[@]}"

declare -A HINTS=(
  ["lint-cpp"]="fix: make format-cpp, recheck: make cpp-format"
  ["lint-yaml"]="fix: make format-yaml, recheck: make yamllint"
  ["lint-markdown"]="fix: make format-markdown, recheck: make markdownlint"
  ["lint-makefile"]="fix: extract target to scripts/, recheck: make lint-makefile"
  ["lint-scripts"]="fix: make format-scripts, recheck: make lint-scripts"
  ["lint-tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["lint-cppcheck"]="fix: address cppcheck warnings, recheck: make lint"
  ["lint-docs"]="fix: address doxygen warnings, recheck: make docs"
  ["lint-complexity"]="fix: refactor complex functions, recheck: make complexity"
  ["build"]="fix: resolve build errors"
  ["unit-test"]="fix: failing tests, recheck: make test"
  ["e2e-test"]="fix: failing e2e tests, recheck: make e2e"
  ["test-coverage"]="fix: add tests, recheck: make coverage-folder"
  ["sast-security"]="fix: address semgrep findings, recheck: make sast-security"
  ["comment-ratio"]="fix: add comments to source"
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
