#!/usr/bin/env bash
# prepush-check.sh — Validate all checks before pushing.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

PASSED=0
FAILED=0
STEP=0
FAILED_NAMES=()

STEPS=(
  # lint-code: precommit already runs format-code (auto-fix)
  # "Lint|lint-code|make -s lint-code"
  # lint-yaml: precommit already runs format-yaml (auto-fix)
  # "Lint|lint-yaml|make -s lint-yaml"
  # lint-md: precommit already runs format-md (auto-fix)
  # "Lint|lint-md|make -s lint-md"
  "Lint|lint-makefile|make -s lint-makefile"
  "Lint|lint-scripts|make -s lint-scripts"
  "Analysis|tidy|make -s tidy"
  # complexity: redundant with tidy + CI runs full-check on main
  # "Analysis|complexity|make -s complexity"
  "Build|build|make -s build"
  "Test|test-unit|make -s test-unit"
  "Test|e2e|make -s e2e"
  # coverage-report: slow (~30s), CI handles this on every PR
  # "Test|coverage-report|make -s coverage-report"
  "Security|sast-security|make -s sast-security"
  # sast-secret: precommit already runs this
  # "Security|sast-secret|make -s sast-secret"
  "Metrics|comment-ratio|make -s comment-ratio"
)

TOTAL="${#STEPS[@]}"

declare -A HINTS=(
  ["lint-code"]="fix: make format-code, recheck: make lint-code"
  ["lint-yaml"]="fix: make format-yaml, recheck: make lint-yaml"
  ["lint-md"]="fix: make format-md, recheck: make lint-md"
  ["lint-makefile"]="fix: follow Makefile conventions, recheck: make lint-makefile"
  ["lint-scripts"]="fix: follow shell script conventions, recheck: make lint-scripts"
  ["tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["complexity"]="fix: refactor complex functions, recheck: make complexity"
  ["build"]="fix: resolve build errors, recheck: make build"
  ["test-unit"]="fix: failing unit tests, recheck: make test-unit"
  ["e2e"]="fix: failing e2e tests, recheck: make e2e"
  ["coverage-report"]="fix: add tests for better coverage, recheck: make coverage-report"
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
