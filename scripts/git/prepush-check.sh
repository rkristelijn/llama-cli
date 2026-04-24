#!/usr/bin/env bash
# prepush-check.sh — Validate all checks before pushing (smart: skips unchanged file types).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Detect which file types changed vs main
CHANGED=$(git diff --name-only main...HEAD 2>/dev/null || git diff --name-only HEAD~1 2>/dev/null || true)
HAS_CPP=false; HAS_SH=false

while IFS= read -r f; do
  case "$f" in
    src/*.cpp|src/*.h|CMakeLists.txt|.config/*) HAS_CPP=true ;;
    scripts/*|Makefile) HAS_SH=true ;;
  esac
done <<< "$CHANGED"

# Build the step list dynamically
STEPS=()
$HAS_SH   && STEPS+=("Lint|lint-makefile|make -s lint-makefile")
$HAS_SH   && STEPS+=("Lint|lint-scripts|make -s lint-scripts")
$HAS_CPP  && STEPS+=("Analysis|tidy|make -s tidy")
$HAS_CPP  && STEPS+=("Build|build|make -s build")
$HAS_CPP  && STEPS+=("Test|test-unit|make -s test-unit")
$HAS_CPP  && STEPS+=("Test|e2e|make -s e2e")
STEPS+=("Security|sast-security|make -s sast-security")
$HAS_CPP  && STEPS+=("Metrics|comment-ratio|make -s comment-ratio")

# If nothing code-related changed, minimal checks
if [[ ${#STEPS[@]} -eq 1 ]]; then
  echo ""
  echo "── No code/script changes — running security only ──"
fi

PASSED=0
FAILED=0
STEP=0
FAILED_NAMES=()
TOTAL="${#STEPS[@]}"

declare -A HINTS=(
  ["lint-makefile"]="fix: follow Makefile conventions, recheck: make lint-makefile"
  ["lint-scripts"]="fix: follow shell script conventions, recheck: make lint-scripts"
  ["tidy"]="fix: address clang-tidy warnings, recheck: make tidy"
  ["build"]="fix: resolve build errors, recheck: make build"
  ["test-unit"]="fix: failing unit tests, recheck: make test-unit"
  ["e2e"]="fix: failing e2e tests, recheck: make e2e"
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
