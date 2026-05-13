#!/usr/bin/env bash
#
# pr-sonar.sh — Fetch SonarCloud issues for the current PR.
#
# Shows new issues introduced by the PR, grouped by severity.
# No SONAR_TOKEN needed — uses the public API for PR analysis.
#
# Usage:
#   bash scripts/gh/pr-sonar.sh [PR_NUMBER]
#   make sonar-report ARGS=PR       # via Makefile
#
# @see docs/adr/adr-074-sonarcloud-integration.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
PROJECT="rkristelijn_llama-cli"
API="https://sonarcloud.io/api"

# Detect PR number from arg, gh CLI, or git branch
PR="${1:-}"
if [[ -z "$PR" ]]; then
  PR=$(gh pr view --json number -q '.number' 2>/dev/null || echo "")
fi
if [[ -z "$PR" ]]; then
  echo "Usage: $0 <PR_NUMBER>"
  exit 1
fi

main() {
  echo "==> SonarCloud issues for PR #${PR}"
  echo ""

  # Fetch issues for this PR (public endpoint, no auth needed)
  local response
  response=$(curl -sf "${API}/issues/search?componentKeys=${PROJECT}&pullRequest=${PR}&statuses=OPEN&ps=50&s=SEVERITY&asc=false" 2>/dev/null)

  if [[ -z "$response" ]]; then
    echo "  [skip] Could not reach SonarCloud API"
    return
  fi

  local total
  total=$(echo "$response" | jq '.total')
  echo "  Total new issues: $total"
  echo ""

  if [[ "$total" -eq 0 ]]; then
    echo "  ✓ No issues — clean PR!"
    return
  fi

  # Group by severity
  echo "  ── By Severity ──"
  echo "$response" | jq -r '.issues | group_by(.severity) | .[] | "\(.[0].severity) \(length)"' |
    sort -k2 -rn | while read -r sev count; do
    local icon="[ ]"
    case "$sev" in
    BLOCKER) icon="[!!]" ;;
    CRITICAL) icon="[!]" ;;
    MAJOR) icon="[~]" ;;
    MINOR) icon="[-]" ;;
    *) icon="[ ]" ;;
    esac
    printf "    %s %-10s %d\n" "$icon" "$sev" "$count"
  done
  echo ""

  # Show CRITICAL + MAJOR issues (the ones to fix)
  echo "  ── Issues to Fix ──"
  echo "$response" | jq -r '.issues[] | select(.severity == "CRITICAL" or .severity == "MAJOR") | "\(.severity)|\(.component | split(":")[1])|\(.line // "?")|\(.message[0:80])"' |
    while IFS='|' read -r sev file line msg; do
      local icon="[~]"
      [[ "$sev" == "CRITICAL" ]] && icon="[!]"
      printf "    %s %s:%s\n      %s\n" "$icon" "$file" "$line" "$msg"
    done
  echo ""

  # Show MINOR/INFO as summary only
  local minor_count
  minor_count=$(echo "$response" | jq '[.issues[] | select(.severity == "MINOR" or .severity == "INFO")] | length')
  if [[ "$minor_count" -gt 0 ]]; then
    echo "  ── Minor/Info ($minor_count) ──"
    echo "$response" | jq -r '.issues[] | select(.severity == "MINOR" or .severity == "INFO") | "    [-] \(.component | split(":")[1]):\(.line // "?") \(.message[0:60])"' | head -10
    if [[ "$minor_count" -gt 10 ]]; then
      echo "    ... and $((minor_count - 10)) more"
    fi
  fi

  echo ""
  echo "  Dashboard: https://sonarcloud.io/project/issues?id=${PROJECT}&pullRequest=${PR}"
}

main "$@"
