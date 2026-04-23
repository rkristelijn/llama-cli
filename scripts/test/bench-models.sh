#!/usr/bin/env bash
#
# bench-models.sh — Benchmark all local Ollama models with a standard prompt.
#
# Usage:
#   bash scripts/test/bench-models.sh              # bench all local models
#   bash scripts/test/bench-models.sh gemma4:26b    # bench specific model(s)
#
# Environment:
#   OLLAMA_HOST  Ollama server (default: localhost:11434)
#   PROMPT       Override the benchmark prompt
#
# Output: table with model, size, tok/s, total time, output tokens.
# @see docs/model-bench.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

HOST="${OLLAMA_HOST:-localhost:11434}"
PROMPT="${PROMPT:-Explain the Eisenhower matrix in 3 sentences.}"

# Collect models: from args or all local models
get_models() {
  if [[ $# -gt 0 ]]; then
    echo "$@"
  else
    ollama list | tail -n +2 | awk '{print $1}'
  fi
}

# Bench a single model. Prints one result line.
# Globals: HOST, PROMPT
# Arguments: model name
bench_one() {
  local model="$1"
  local result
  result=$(curl -s "http://${HOST}/api/chat" -d "{
    \"model\": \"${model}\",
    \"stream\": false,
    \"messages\": [{\"role\": \"user\", \"content\": \"${PROMPT}\"}]
  }" 2>&1)

  local tokens duration total tps total_s
  tokens=$(echo "$result" | grep -o '"eval_count":[0-9]*' | cut -d: -f2)
  duration=$(echo "$result" | grep -o '"eval_duration":[0-9]*' | cut -d: -f2)
  total=$(echo "$result" | grep -o '"total_duration":[0-9]*' | cut -d: -f2)

  if [[ -n "${tokens:-}" && -n "${duration:-}" && "${duration:-0}" -gt 0 ]]; then
    tps=$(echo "scale=1; ${tokens} / (${duration} / 1000000000)" | bc)
    total_s=$(echo "scale=1; ${total} / 1000000000" | bc)
  else
    tps="err"
    total_s="err"
  fi

  printf "| %-28s | %6s | %6s | %6s |\n" "$model" "$tps" "${total_s}s" "$tokens"
}

main() {
  local models
  models=$(get_models "$@")

  echo "==> bench-models (prompt: \"${PROMPT:0:50}...\")"
  echo ""
  printf "| %-28s | %6s | %6s | %6s |\n" "Model" "tok/s" "Total" "Tokens"
  printf "| %-28s | %6s | %6s | %6s |\n" "---" "---:" "---:" "---:"

  for model in ${models}; do
    bench_one "$model"
  done

  echo ""
  echo "  [done] bench-models"
}

main "$@"
