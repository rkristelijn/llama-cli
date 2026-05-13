#!/usr/bin/env bash
#
# learn.sh — AI-assisted pattern discovery and consistency enforcement.
#
# Uses llama-cli locally to analyze code, discover patterns, and accumulate
# lessons. Each run has a 5-minute budget. Results are appended to
# .config/lessons.yml and .config/patterns.yml.
#
# USAGE:
#   scripts/dev/learn.sh [mode]
#
# MODES:
#   patterns   Discover code patterns and generate semgrep rules
#   tests      Analyze test failures and extract best practices
#   all        Run all modes (default)
#
# ENVIRONMENT:
#   LEARN_MODEL    Model to use (default: qwen2.5-coder:14b)
#   LEARN_TIMEOUT  Budget in seconds (default: 300)
#   LLAMA_CLI      Path to llama-cli binary (default: ./build/llama-cli)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
# --- Configuration ---
shopt -s globstar 2>/dev/null || true
LEARN_TIMEOUT="${LEARN_TIMEOUT:-300}"
LLAMA_CLI="${LLAMA_CLI:-./llama-cli}"
LESSONS_FILE=".config/lessons.yml"
PATTERNS_FILE=".config/patterns.yml"
MODE="${1:-all}"
START_TIME=$(date +%s)

# --- Helpers ---

# Check if time budget is exhausted
budget_ok() {
  local elapsed=$(($(date +%s) - START_TIME))
  [[ $elapsed -lt $LEARN_TIMEOUT ]]
}

# Ask llama-cli a question, return the response
ask_llm() {
  local prompt="$1"
  local system="${2:-Be concise. Output YAML only. No markdown fences.}"
  if [[ ! -x "$LLAMA_CLI" ]]; then
    echo "  [skip] llama-cli not found at $LLAMA_CLI" >&2
    return 1
  fi
  "$LLAMA_CLI" --system-prompt="$system" "$prompt" 2>/dev/null
}

# Append a lesson to lessons.yml if not already present
add_lesson() {
  local category="$1"
  local lesson="$2"
  # Skip if already recorded
  if grep -qF "$lesson" "$LESSONS_FILE" 2>/dev/null; then
    return 0
  fi
  printf "  - %s\n" "$lesson" >>"$LESSONS_FILE"
  echo "  [learned] $lesson"
}

# --- Modes ---

# Analyze source code for decomposition opportunities
learn_patterns() {
  echo "==> learn: discovering code patterns..."

  # Find functions with high complexity
  local complex_funcs
  complex_funcs=$(pmccabe src/**/*.cpp 2>/dev/null |
    awk '$1 > 8 {print $NF ":" $6 " (complexity=" $1 ")"}' |
    head -5 || true)

  if [[ -z "$complex_funcs" ]]; then
    echo "  [ok] no high-complexity functions found"
    return 0
  fi

  budget_ok || return 0

  # Ask LLM to suggest Extract Method opportunities
  local prompt
  prompt="These C++ functions have high cyclomatic complexity:
${complex_funcs}

Analyze and suggest semgrep rules (YAML format) to detect:
1. Complex inline conditions that should be extracted
2. Functions mixing abstraction levels
3. Long parameter lists suggesting missing abstractions

Output valid semgrep rules YAML only. Use id prefix 'learn-'."

  local response
  response=$(ask_llm "$prompt") || return 0

  # Append new rules if they look valid
  if echo "$response" | grep -q "id:"; then
    {
      echo ""
      echo "# Learned $(date +%Y-%m-%d) from complexity analysis"
      echo "$response"
    } >>"$PATTERNS_FILE"
    echo "  [pattern] added new rules to $PATTERNS_FILE"
  fi
}

# Analyze recent test failures for lessons
learn_tests() {
  echo "==> learn: analyzing test patterns..."

  # Find recent test files
  local test_files
  test_files=$(find src -name "*_test.cpp" -newer "$LESSONS_FILE" 2>/dev/null | head -5)
  if [[ -z "$test_files" ]]; then
    test_files=$(find src -name "*_test.cpp" | head -5)
  fi

  budget_ok || return 0

  # Look for patterns: TODO/FIXME in tests, timing-sensitive assertions
  local issues=""
  for f in $test_files; do
    # Detect sleep-based tests
    if grep -q "sleep" "$f" 2>/dev/null; then
      issues+="$f: uses sleep (timing-sensitive)\n"
    fi
    # Detect empty() assertions after side-effects
    if grep -q "\.empty()" "$f" 2>/dev/null; then
      issues+="$f: empty() assertion — verify it tests contract not implementation\n"
    fi
  done

  if [[ -n "$issues" ]]; then
    local prompt
    prompt="These test patterns were found in a C++ project using doctest:
$(printf '%b' "$issues")

For each, suggest a one-line lesson (best practice) to add to our lessons database.
Format: category: lesson text
Categories: timing, assertions, isolation, naming"

    local response
    response=$(ask_llm "$prompt" "Be concise. One lesson per line. Format: 'category: lesson'") || return 0

    # Parse and add lessons
    while IFS=: read -r category lesson; do
      [[ -n "$lesson" ]] && add_lesson "${category## }" "${lesson## }"
    done <<<"$response"
  else
    echo "  [ok] no test anti-patterns detected"
  fi
}

# --- Main ---

main() {
  echo "==> learn (budget=${LEARN_TIMEOUT}s, mode=$MODE)"

  # Ensure output files exist
  if [[ ! -f "$LESSONS_FILE" ]]; then
    cat >"$LESSONS_FILE" <<'EOF'
# lessons.yml — Accumulated code and test lessons (auto-generated by learn.sh)
# Each entry is a best practice discovered through analysis.
#
# Format:
#   category:
#     - lesson text
#
timing:
  - Use output-producing commands for timeout tests, not sleep
  - Coverage builds add ~2x overhead — assertions must account for this
assertions:
  - Test the contract (observable behavior), not implementation details
  - json_extract_string handles numeric values as string representation
isolation:
  - Tests must pass in: local macOS, CI Ubuntu, coverage build, sanitizer build
  - No filesystem side-effects without cleanup in THEN blocks
naming:
  - Test names read as documentation: SCENARIO("module: behavior description")
EOF
  fi

  if [[ ! -f "$PATTERNS_FILE" ]]; then
    cat >"$PATTERNS_FILE" <<'EOF'
# patterns.yml — Semgrep rules learned from codebase analysis
# Run with: semgrep scan --config .config/patterns.yml
#
# Rules are auto-discovered by scripts/dev/learn.sh and manually curated.
rules:
  - id: learn-extract-complex-condition
    patterns:
      - pattern: |
          if ($X && $Y && $Z) { ... }
    message: "Complex condition with 3+ clauses — consider Extract Method (ADR-076)"
    languages: [cpp]
    severity: WARNING

  - id: learn-no-sleep-in-tests
    patterns:
      - pattern: |
          cmd_exec("sleep $X", ...)
    message: "sleep in tests is timing-sensitive — use output-producing commands (ADR-075)"
    languages: [cpp]
    severity: ERROR
    paths:
      include:
        - "*_test.cpp"
EOF
  fi

  case "$MODE" in
  patterns) learn_patterns ;;
  tests) learn_tests ;;
  all)
    learn_patterns
    budget_ok && learn_tests
    ;;
  *)
    echo "Unknown mode: $MODE (use: patterns, tests, all)" >&2
    exit 1
    ;;
  esac

  local elapsed=$(($(date +%s) - START_TIME))
  echo "  [done] learn (${elapsed}s elapsed)"
}

main "$@"
