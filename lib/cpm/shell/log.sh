#!/usr/bin/env bash
# Centralized check log — appends results with timestamp + commit hash.
# Enables looking back in time to see maturity progression.
# @see docs/adr/011-020/021-cmmi-mapped-quality-matrix.md
#
# Usage: source this, then call log_run at end of any gate.
#   log_run "check-fast" 0    # gate name, exit code
#   log_run "pre-commit" 1    # failed

LOG_FILE=".tmp/checks.log"
mkdir -p .tmp

log_run() {
  local gate="$1"
  local exit_code="${2:-0}"
  local ts=$(date +%Y-%m-%dT%H:%M:%S%z)
  local hash=$(git rev-parse --short HEAD 2>/dev/null || echo "none")
  local branch=$(git symbolic-ref --short HEAD 2>/dev/null || echo "detached")
  local status="pass"
  [[ "$exit_code" -ne 0 ]] && status="fail"

  # Count active/skipped from registry
  local active=$(jq '[.checks[] | select(.skip.enabled != true)] | length' .config/checks-registry.json 2>/dev/null || echo "?")
  local total=$(jq '.checks | length' .config/checks-registry.json 2>/dev/null || echo "?")

  printf "%s | %s | %s | %s | %s | %s/%s active\n" \
    "$ts" "$hash" "$branch" "$gate" "$status" "$active" "$total" >> "$LOG_FILE"
}

# Show recent log entries
log_show() {
  local n="${1:-10}"
  [[ -f "$LOG_FILE" ]] || { echo "No log yet."; return; }
  echo "  Recent check runs:"
  echo "  ─────────────────────────────────────────────────────────"
  tail -n "$n" "$LOG_FILE" | while IFS='|' read -r ts hash branch gate status active; do
    printf "  %s %s %-12s %-12s %s %s\n" "$ts" "$hash" "$branch" "$gate" "$status" "$active"
  done
}
