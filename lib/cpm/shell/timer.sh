#!/usr/bin/env bash
# timer.sh — Measure check durations with trend comparison.
#
# Tracks timing per check, compares to last run, warns on regressions.
# Data stored in .tmp/timings.jsonl (one JSON line per check per run).
#
# Usage:
#   source lib/cpm/shell/timer.sh
#   timer_start "lint-code"
#   ... do work ...
#   timer_stop "lint-code"    # prints duration + trend
#   timer_summary             # prints total + comparison
#
# @see docs/adr/adr-121-cpm-quality-layer.md

TIMINGS_FILE="${CPM_TIMINGS_FILE:-.tmp/timings.jsonl}"
mkdir -p "$(dirname "$TIMINGS_FILE")"

# Store start time in a temp file (works with bash 3.2, no associative arrays)
_CPM_TIMER_DIR=".tmp/timers"
mkdir -p "$_CPM_TIMER_DIR"

# Start timing a check
timer_start() {
  local name="$1"
  date +%s%N > "$_CPM_TIMER_DIR/$name"
}

# Stop timing, print result, log to file
timer_stop() {
  local name="$1"
  local status="${2:-success}"
  local now
  now=$(date +%s%N)
  local start
  start=$(cat "$_CPM_TIMER_DIR/$name" 2>/dev/null || echo "$now")
  local duration_ms=$(( (now - start) / 1000000 ))

  # Get previous duration for this check
  local prev_ms=""
  if [[ -f "$TIMINGS_FILE" ]]; then
    prev_ms=$(grep "\"name\":\"$name\"" "$TIMINGS_FILE" | tail -1 | grep -o '"ms":[0-9]*' | cut -d: -f2)
  fi

  # Format duration
  local duration_str
  if ((duration_ms < 1000)); then
    duration_str="${duration_ms}ms"
  else
    duration_str="$(( duration_ms / 1000 )).$(( (duration_ms % 1000) / 100 ))s"
  fi

  # Trend indicator
  local trend=""
  if [[ -n "$prev_ms" && "$prev_ms" -gt 0 ]]; then
    local diff=$(( duration_ms - prev_ms ))
    local pct=$(( (diff * 100) / prev_ms ))
    if ((pct > 20)); then
      trend=" ↑${pct}% slower"
    elif ((pct < -20)); then
      trend=" ↓$(( -pct ))% faster"
    fi
  fi

  # Print with color if ui.sh is loaded
  if declare -f print_step >/dev/null 2>&1; then
    print_step "" "$name" "$status" "${duration_str}${trend}"
  else
    echo "  $name $status ${duration_str}${trend}"
  fi

  # Log to JSONL
  local ts
  ts=$(date +%Y-%m-%dT%H:%M:%S%z)
  printf '{"ts":"%s","name":"%s","ms":%d,"status":"%s"}\n' \
    "$ts" "$name" "$duration_ms" "$status" >> "$TIMINGS_FILE"
}

# Print total duration and comparison to last full run
timer_summary() {
  local gate="${1:-check}"
  local total_ms=0
  local count=0

  # Sum all timings from this session (last N entries matching current timestamp prefix)
  local today
  today=$(date +%Y-%m-%dT%H:%M)
  if [[ -f "$TIMINGS_FILE" ]]; then
    while IFS= read -r line; do
      local ms
      ms=$(echo "$line" | grep -o '"ms":[0-9]*' | cut -d: -f2)
      total_ms=$((total_ms + ms))
      count=$((count + 1))
    done < <(grep "$today" "$TIMINGS_FILE" | tail -"${count:-50}")
  fi

  # Get previous total
  local prev_total=""
  if [[ -f ".tmp/last-${gate}-ms" ]]; then
    prev_total=$(cat ".tmp/last-${gate}-ms")
  fi

  # Format
  local total_str
  if ((total_ms < 1000)); then
    total_str="${total_ms}ms"
  else
    total_str="$(( total_ms / 1000 )).$(( (total_ms % 1000) / 100 ))s"
  fi

  local trend=""
  if [[ -n "$prev_total" && "$prev_total" -gt 0 ]]; then
    local diff=$(( total_ms - prev_total ))
    local pct=$(( (diff * 100) / prev_total ))
    if ((pct > 10)); then
      trend=" (↑${pct}% vs last run)"
    elif ((pct < -10)); then
      trend=" (↓$(( -pct ))% faster)"
    fi
  fi

  echo ""
  echo "  Total: ${total_str}${trend} (${count} checks)"

  # Save for next comparison
  echo "$total_ms" > ".tmp/last-${gate}-ms"
}
