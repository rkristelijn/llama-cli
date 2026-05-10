#!/usr/bin/env bash
#
# preflight.sh — Quick benchmark to assess model capabilities before use.
#
# Tests: math, factual recall, reasoning, vague/creative questions.
# Outputs a scorecard per model showing speed and accuracy.
#
# Usage:
#   bash scripts/test/preflight.sh [host1:port host2:port ...]
#   bash scripts/test/preflight.sh                          # uses OLLAMA_HOSTS or defaults
#
# Environment:
#   OLLAMA_HOSTS  Comma-separated host:port list (default: localhost:11434)
#   BINARY        Path to llama-cli binary (default: ./build/llama-cli)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BINARY="${BINARY:-./build/llama-cli}"

# --- Test questions with expected answers ---
# Format: "question|expected_pattern" (grep -i pattern)
TESTS=(
  "What is 7 times 8?|56"
  "What is the capital of France?|paris"
  "What is the capital of Japan?|tokyo"
  "If I have 3 apples and give away 1, how many do I have?|2"
  "What color is the sky on a clear day?|blue"
  "Complete: the quick brown fox jumps over the|lazy"
)

# Vague questions — no right answer, just check for non-empty response
VAGUE_TESTS=(
  "What is your favorite color?"
  "What is the house number of your last name?"
  "If a tree falls in a forest and no one hears it, does it make a sound?"
)

# --- Discover hosts and models ---
discover_hosts() {
  if [[ $# -gt 0 ]]; then
    echo "$@"
    return
  fi
  if [[ -n "${OLLAMA_HOSTS:-}" ]]; then
    echo "${OLLAMA_HOSTS}" | tr ',' ' '
    return
  fi
  # Default: check common hosts
  local hosts=""
  for h in "localhost:11434" "$(hostname).local:11434" "$(hostname):11434"; do
    local host="${h%%:*}"
    local port="${h##*:}"
    if curl -s --connect-timeout 2 "http://${host}:${port}/api/tags" | grep -q "models" 2>/dev/null; then
      hosts="${hosts} ${h}"
    fi
  done
  echo "${hosts}"
}

get_models() {
  local host="${1%%:*}"
  local port="${1##*:}"
  curl -s --connect-timeout 3 "http://${host}:${port}/api/tags" 2>/dev/null |
    python3 -c "import sys,json; [print(m['name']) for m in json.loads(sys.stdin.read()).get('models',[])]" 2>/dev/null
}

# --- Run a single test ---
# Returns: "pass|fail duration_ms"
run_test() {
  local host="$1" model="$2" question="$3" expected="$4"
  local h="${host%%:*}"
  local start end ms result

  start=$(date +%s%N)
  result=$(OLLAMA_SYSTEM_PROMPT="Answer in one word or number only. No explanation." \
    "${BINARY}" --host="${h}" --model="${model}" "${question}" 2>/dev/null | head -1)
  end=$(date +%s%N)
  ms=$(((end - start) / 1000000))

  if echo "${result}" | grep -qi "${expected}"; then
    echo "pass ${ms}"
  else
    echo "fail ${ms} got:${result}"
  fi
}

# --- Run vague test (just check non-empty response) ---
run_vague_test() {
  local host="$1" model="$2" question="$3"
  local h="${host%%:*}"
  local start end ms result

  start=$(date +%s%N)
  result=$(OLLAMA_SYSTEM_PROMPT="Be creative and brief." \
    "${BINARY}" --host="${h}" --model="${model}" "${question}" 2>/dev/null | head -3)
  end=$(date +%s%N)
  ms=$(((end - start) / 1000000))

  if [[ -n "${result}" ]]; then
    echo "pass ${ms}"
  else
    echo "fail ${ms}"
  fi
}

# --- Main ---
main() {
  local hosts
  hosts=$(discover_hosts "$@")

  if [[ -z "${hosts}" ]]; then
    echo "No Ollama hosts found. Set OLLAMA_HOSTS or pass host:port arguments."
    exit 1
  fi

  echo "╔══════════════════════════════════════════════════════════════╗"
  echo "║              PREFLIGHT MODEL BENCHMARK                      ║"
  echo "╚══════════════════════════════════════════════════════════════╝"
  echo ""

  for host in ${hosts}; do
    echo "━━━ ${host} ━━━"
    local models
    models=$(get_models "${host}")
    if [[ -z "${models}" ]]; then
      echo "  (no models or offline)"
      continue
    fi

    for model in ${models}; do
      # Skip embedding models
      if echo "${model}" | grep -qi "embed"; then continue; fi

      local pass=0 fail=0 total_ms=0
      printf "  %-40s " "${model}"

      # Factual tests
      for test in "${TESTS[@]}"; do
        local question="${test%%|*}"
        local expected="${test##*|}"
        local result
        result=$(run_test "${host}" "${model}" "${question}" "${expected}")
        local status="${result%% *}"
        local ms="${result#* }"
        ms="${ms%% *}"
        if [[ "${status}" == "pass" ]]; then
          ((pass++)) || true
        else
          ((fail++)) || true
        fi
        total_ms=$((total_ms + ms))
      done

      # Vague tests
      for question in "${VAGUE_TESTS[@]}"; do
        local result
        result=$(run_vague_test "${host}" "${model}" "${question}")
        local status="${result%% *}"
        local ms="${result#* }"
        ms="${ms%% *}"
        if [[ "${status}" == "pass" ]]; then
          ((pass++)) || true
        fi
        total_ms=$((total_ms + ms))
      done

      local total=$((pass + fail))
      local avg_ms=$((total_ms / (total + ${#VAGUE_TESTS[@]})))
      printf "%d/%d correct  avg %dms\n" "${pass}" "$((total + ${#VAGUE_TESTS[@]}))" "${avg_ms}"
    done
    echo ""
  done
}

main "$@"
