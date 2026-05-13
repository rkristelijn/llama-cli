#!/usr/bin/env bash
# cpm-check.sh — Orchestrator: runs checks from registry with delta detection.
#
# Usage:
#   bash lib/cpm/shell/cpm-check.sh [fast|normal|full]
#
# Responsibilities (single): coordinate modules, report results.
# Delegates to: delta.sh (what changed), registry.sh (what to run),
#               run.sh (execute + time), junit.sh (report).
#
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o nounset
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/init.sh"
source "$SCRIPT_DIR/delta.sh"
source "$SCRIPT_DIR/registry.sh"
source "$SCRIPT_DIR/junit.sh"

TIER="${1:-normal}"
CPM_RUN_MODE="${CPM_RUN_MODE:-collect}"

# Counters
_total=0 _passed=0 _failed=0 _skipped=0
_errors=""
_changes=""

# Handler called for each check in the registry
handle_check() {
  local name="$1" command="$2" triggers="$3" scope="$4" severity="$5"
  _total=$((_total + 1))

  # Delta detection
  if [[ "$scope" != "full" ]] && ! cpm_triggers_match "$triggers" "$_changes"; then
    _skipped=$((_skipped + 1))
    print_step "" "$name" skip "no matching changes"
    junit_testcase "$name" "skip" "0" "no matching changes"
    return
  fi

  # Execute
  local start; start=$(date +%s%N)
  if bash "$SCRIPT_DIR/run.sh" "$name" bash "$command" 2>/dev/null; then
    local dur=$(( ($(date +%s%N) - start) / 1000000 ))
    _passed=$((_passed + 1))
    junit_testcase "$name" "success" "$dur" ""
  else
    local dur=$(( ($(date +%s%N) - start) / 1000000 ))
    if [[ "$severity" == "error" ]]; then
      _failed=$((_failed + 1))
      _errors+="  ✗ $name (severity: error)\n"
      junit_testcase "$name" "error" "$dur" "check failed"
      [[ "$CPM_RUN_MODE" == "fail-fast" ]] && { junit_finish; printf "%b" "$_errors"; exit 1; }
    else
      _passed=$((_passed + 1))
      junit_testcase "$name" "success" "$dur" ""
    fi
  fi
}

main() {
  _changes=$(cpm_changed_files)

  junit_start "cpm"
  print_header "cpm check (tier: $TIER)"

  cpm_parse_checks handle_check

  junit_finish

  # Summary
  echo ""
  if [[ $_failed -eq 0 ]]; then
    print_summary "$_total checks ($_passed passed, $_skipped skipped)"
  else
    echo -e "  ${RED:-}${_failed} failed${RESET:-}, $_passed passed, $_skipped skipped ($_total total)"
    [[ -n "$_errors" ]] && printf "\n%b" "$_errors"
    exit 1
  fi
}

main
