#!/usr/bin/env bash
#
# log-viewer.sh — Display event logs as a clean table
# Usage: ./log-viewer.sh [pattern] [-n COUNT]
#   pattern  — grep filter on raw JSONL lines (default: show all)
#   -n COUNT — number of recent events to show (default: 50)

set -o errexit
set -o nounset
set -o pipefail

LOG_FILE="${LLAMA_LOG_FILE:-$HOME/.llama-cli/events.jsonl}"
COUNT=50
PATTERN=""

while [ $# -gt 0 ]; do
  case "$1" in
    -n) COUNT="$2"; shift 2 ;;
    -h|--help)
      echo "Usage: $0 [pattern] [-n COUNT]"
      echo "  pattern   Show only events matching this string"
      echo "  -n COUNT  Number of recent events (default: 50)"
      echo "  LLAMA_LOG_FILE env var overrides default log path"
      exit 0 ;;
    *) PATTERN="$1"; shift ;;
  esac
done

if [ ! -f "$LOG_FILE" ]; then
  echo "No log file: $LOG_FILE"
  exit 1
fi

# Extract field value from JSON line (no jq dependency)
field() {
  echo "$1" | sed -n "s/.*\"$2\":\"\{0,1\}\([^,\"}]*\)\"\{0,1\}.*/\1/p"
}

# Header
printf "%-10s %-10s %-18s %8s %6s %6s  %s\n" "TIME" "AGENT" "ACTION" "DURATION" "IN" "OUT" "SUMMARY"
printf "%-10s %-10s %-18s %8s %6s %6s  %s\n" "----------" "----------" "------------------" "--------" "------" "------" "-------"

# Select lines
if [ -n "$PATTERN" ]; then
  data=$(grep -- "$PATTERN" "$LOG_FILE" | tail -"$COUNT")
else
  data=$(tail -"$COUNT" "$LOG_FILE")
fi

echo "$data" | while IFS= read -r line; do
  [ -z "$line" ] && continue
  ts=$(field "$line" timestamp | sed 's/.*T//;s/\..*//')
  agent=$(field "$line" agent)
  action=$(field "$line" action)
  dur=$(field "$line" duration_ms)
  tp=$(field "$line" tokens_prompt)
  tc=$(field "$line" tokens_completion)
  input=$(echo "$line" | sed -n 's/.*"input":"\([^"]*\)".*/\1/p' | cut -c1-50)

  printf "%-10s %-10s %-18s %6sms %6s %6s  %s\n" \
    "$ts" "$agent" "$action" "${dur:-0}" "${tp:-0}" "${tc:-0}" "$input"
done
