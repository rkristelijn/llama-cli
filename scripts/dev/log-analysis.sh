#!/usr/bin/env bash
#
# log-analysis.sh — Analyze llama-cli event logs: timing, models, features, errors.
#
# Usage:
#   bash scripts/dev/log-analysis.sh [LOG_FILE] [--last N]
#
# Defaults to .tmp/events.jsonl (dev mode) or ~/.llama-cli/events.jsonl (installed).
#
# @see docs/adr/adr-027-event-logging.md

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
LOG_FILE="${1:-.tmp/events.jsonl}"
LAST=""

# Parse args
shift 2>/dev/null || true
while [[ $# -gt 0 ]]; do
  case "$1" in
  --last)
    LAST="$2"
    shift 2
    ;;
  *) shift ;;
  esac
done

if [[ ! -f "$LOG_FILE" ]]; then
  LOG_FILE="$HOME/.llama-cli/events.jsonl"
fi
if [[ ! -f "$LOG_FILE" ]]; then
  echo "No log file found."
  exit 1
fi

main() {
  local data="$LOG_FILE"
  local total
  total=$(wc -l <"$data")

  # Apply --last filter
  local input="cat $data"
  if [[ -n "$LAST" ]]; then
    input="tail -n $LAST $data"
  fi

  echo "==> llama-cli Log Analysis"
  echo "    File: $data ($total events)"
  echo ""

  # --- Sessions ---
  echo "── Sessions ──"
  local sessions
  sessions=$(eval "$input" | jq -r 'select(.action == "session_start") | .input' | wc -l)
  printf "  Total sessions: %d\n" "$sessions"
  echo "  Models used:"
  eval "$input" | jq -r 'select(.action == "session_start") | .input' | sort | uniq -c | sort -rn | head -5 |
    while read -r count model; do printf "    %4dx  %s\n" "$count" "$model"; done
  echo ""

  # --- Chat performance ---
  echo "── Chat Performance ──"
  local chats avg_dur avg_tps
  chats=$(eval "$input" | jq -r 'select(.action == "chat_stream" and .duration_ms > 0)' | jq -s 'length')
  if [[ "$chats" -gt 0 ]]; then
    avg_dur=$(eval "$input" | jq -r 'select(.action == "chat_stream" and .duration_ms > 0) | .duration_ms' |
      awk '{sum+=$1; n++} END {printf "%.0f", sum/n}')
    avg_tps=$(eval "$input" | jq -r 'select(.action == "chat_stream" and .tokens_completion > 0) | (.tokens_completion / (.duration_ms / 1000))' |
      awk '{sum+=$1; n++} END {if(n>0) printf "%.1f", sum/n; else print "0"}')
    local p95_dur
    p95_dur=$(eval "$input" | jq -r 'select(.action == "chat_stream" and .duration_ms > 0) | .duration_ms' |
      sort -n | awk -v p=0.95 'BEGIN{c=0} {v[c++]=$1} END{printf "%.0f", v[int(c*p)]}')
    printf "  Chats: %d | Avg: %sms | P95: %sms | Avg tok/s: %s\n" "$chats" "$avg_dur" "$p95_dur" "$avg_tps"
  else
    echo "  No chat events found."
  fi
  echo ""

  # --- Actions ---
  echo "── Actions (top 10) ──"
  eval "$input" | jq -r '.action' | sort | uniq -c | sort -rn | head -10 |
    while read -r count act; do printf "  %4dx  %s\n" "$count" "$act"; done
  echo ""

  # --- Agents ---
  echo "── Agents ──"
  eval "$input" | jq -r '.agent' | sort | uniq -c | sort -rn |
    while read -r count agent; do printf "  %4dx  %s\n" "$count" "$agent"; done
  echo ""

  # --- Errors ---
  echo "── Errors ──"
  local errors
  errors=$(eval "$input" | jq -r 'select(.action == "error" or (.output | test("error|Error|failed|timeout"; "i")))' 2>/dev/null | jq -s 'length')
  printf "  Total error events: %d\n" "${errors:-0}"
  eval "$input" | jq -r 'select(.action == "error") | "\(.timestamp) \(.output[0:80])"' 2>/dev/null | tail -5 |
    while read -r line; do echo "    $line"; done
  echo ""

  # --- Exec commands ---
  echo "── Exec (top 5 by duration) ──"
  eval "$input" | jq -r 'select(.action == "exec" and .duration_ms > 0) | "\(.duration_ms)ms \(.input[0:60])"' 2>/dev/null |
    sort -rn | head -5 | while read -r line; do echo "    $line"; done
  echo ""

  # --- Timeline (last 7 days) ---
  echo "── Activity (last 7 days) ──"
  eval "$input" | jq -r '.timestamp[0:10]' | sort | uniq -c | tail -7 |
    while read -r count day; do
      local bar=""
      local blocks=$((count / 20))
      for ((i = 0; i < blocks && i < 40; i++)); do bar+="█"; done
      printf "  %s  %4d  %s\n" "$day" "$count" "$bar"
    done
}

main "$@"
