#!/usr/bin/env bash
#
# ci-analysis.sh — Analyze CI pipeline: timing, failures, success rates.
#
# Usage:
#   bash scripts/gh/ci-analysis.sh [--runs N] [--branch BRANCH]
#
# Requires: gh CLI authenticated
#
# @see docs/adr/adr-002-quality-checks.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
RUNS=10
BRANCH=""

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
  --runs)
    RUNS="$2"
    shift 2
    ;;
  --branch)
    BRANCH="$2"
    shift 2
    ;;
  *)
    echo "Unknown arg: $1"
    exit 1
    ;;
  esac
done

main() {
  echo "==> CI Pipeline Analysis (last ${RUNS} runs)"
  echo ""

  # Build gh args
  local gh_args=(--limit "$RUNS" --json "databaseId,conclusion,status,createdAt,updatedAt")
  if [[ -n "$BRANCH" ]]; then
    gh_args+=(--branch "$BRANCH")
  fi

  # Fetch runs
  local runs
  runs=$(gh run list "${gh_args[@]}" 2>/dev/null)
  if [[ -z "$runs" ]]; then
    echo "  No runs found."
    return
  fi

  # Summary stats
  local total success failure cancelled
  total=$(echo "$runs" | jq 'length')
  success=$(echo "$runs" | jq '[.[] | select(.conclusion == "success")] | length')
  failure=$(echo "$runs" | jq '[.[] | select(.conclusion == "failure")] | length')
  cancelled=$(echo "$runs" | jq '[.[] | select(.conclusion == "cancelled")] | length')

  echo "── Summary ──"
  printf "  Total: %d | [PASS] Success: %d | [FAIL] Failure: %d | ⏹ Cancelled: %d\n" "$total" "$success" "$failure" "$cancelled"
  if [[ "$total" -gt 0 ]]; then
    local rate=$((success * 100 / total))
    echo "  Success rate: ${rate}%"
  fi
  echo ""

  # Latest run job details
  local latest_id
  latest_id=$(echo "$runs" | jq -r '.[0].databaseId')
  echo "── Latest Run (#${latest_id}) ──"

  local jobs
  jobs=$(gh run view "$latest_id" --json jobs 2>/dev/null)

  # Job timing table
  echo ""
  printf "  %-25s %-10s %s\n" "JOB" "STATUS" "DURATION"
  printf "  %-25s %-10s %s\n" "---" "------" "--------"

  echo "$jobs" | jq -r '.jobs[] | select(.conclusion != "skipped") | [.name, .conclusion, .startedAt, .completedAt] | @tsv' |
    while IFS=$'\t' read -r name conclusion started completed; do
      # Calculate duration in seconds
      local dur="?"
      if [[ -n "$started" && -n "$completed" && "$started" != "null" && "$completed" != "null" ]]; then
        local s_epoch c_epoch
        s_epoch=$(date -d "$started" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%SZ" "$started" +%s 2>/dev/null || echo 0)
        c_epoch=$(date -d "$completed" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%SZ" "$completed" +%s 2>/dev/null || echo 0)
        if [[ "$s_epoch" -gt 0 && "$c_epoch" -gt 0 ]]; then
          dur="$((c_epoch - s_epoch))s"
        fi
      fi
      local icon="[PASS]"
      [[ "$conclusion" == "failure" ]] && icon="[FAIL]"
      [[ "$conclusion" == "cancelled" ]] && icon="⏹"
      printf "  %-25s %s %-8s %s\n" "$name" "$icon" "$conclusion" "$dur"
    done

  # Failure analysis
  echo ""
  echo "── Failure Patterns (last ${RUNS} runs) ──"
  local all_jobs=""
  for run_id in $(echo "$runs" | jq -r '.[].databaseId'); do
    local rjobs
    rjobs=$(gh run view "$run_id" --json jobs 2>/dev/null | jq -r '.jobs[] | select(.conclusion == "failure") | .name' 2>/dev/null)
    if [[ -n "$rjobs" ]]; then
      all_jobs="${all_jobs}${rjobs}"$'\n'
    fi
  done

  if [[ -n "$all_jobs" ]]; then
    echo "$all_jobs" | sort | uniq -c | sort -rn | head -10 | while read -r count name; do
      printf "  %3dx  %s\n" "$count" "$name"
    done
  else
    echo "  No failures found :)"
  fi

  # Slowest jobs (from latest run)
  echo ""
  echo "── Slowest Jobs ──"
  echo "$jobs" | jq -r '.jobs[] | select(.conclusion != "skipped") | [.name, .startedAt, .completedAt] | @tsv' |
    while IFS=$'\t' read -r name started completed; do
      if [[ -n "$started" && -n "$completed" && "$started" != "null" && "$completed" != "null" ]]; then
        local s_epoch c_epoch
        s_epoch=$(date -d "$started" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%SZ" "$started" +%s 2>/dev/null || echo 0)
        c_epoch=$(date -d "$completed" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%SZ" "$completed" +%s 2>/dev/null || echo 0)
        if [[ "$s_epoch" -gt 0 && "$c_epoch" -gt 0 ]]; then
          echo "$((c_epoch - s_epoch)) $name"
        fi
      fi
    done | sort -rn | head -5 | while read -r secs name; do
    printf "  %3ds  %s\n" "$secs" "$name"
  done
}

main "$@"
