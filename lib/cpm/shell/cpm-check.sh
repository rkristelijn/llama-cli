#!/usr/bin/env bash
# cpm-check.sh — Run checks based on registry, delta detection, and tier.
#
# Usage:
#   bash lib/cpm/shell/cpm-check.sh [tier]
#   tier: fast | normal | full (default: normal)
#
# Reads [[checks]] from cpm.toml, detects changed files, skips irrelevant checks.
#
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o nounset
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ui.sh"

TIER="${1:-normal}"
CPM_RUN_MODE="${CPM_RUN_MODE:-collect}"

# Get changed files vs main
changed_files() {
  git diff --name-only main...HEAD 2>/dev/null || git diff --name-only HEAD~1 2>/dev/null || find src scripts -type f
}

# Check if any trigger pattern matches changed files
triggers_match() {
  local triggers="$1"
  local changes="$2"

  # If triggers contain "**" (match all), always run
  [[ "$triggers" == *'**'* ]] && echo "$triggers" | grep -q '"\*\*"' && return 0

  # Check each changed file against trigger patterns
  while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    # Simple glob matching
    case "$triggers" in
      *"src/"*) [[ "$file" == src/* ]] && return 0 ;;
      *"scripts/"*) [[ "$file" == scripts/* ]] && return 0 ;;
      *".md"*) [[ "$file" == *.md ]] && return 0 ;;
      *".yml"*|*".yaml"*) [[ "$file" == *.yml || "$file" == *.yaml ]] && return 0 ;;
      *".sh"*) [[ "$file" == *.sh ]] && return 0 ;;
      *"CMakeLists"*) [[ "$file" == *CMakeLists* ]] && return 0 ;;
      *"e2e/"*) [[ "$file" == e2e/* ]] && return 0 ;;
    esac
  done <<< "$changes"
  return 1
}

# Parse checks from cpm.toml (simple grep-based, no deps)
run_checks() {
  local changes
  changes=$(changed_files)
  local total=0 passed=0 failed=0 skipped=0
  local errors=""

  print_header "cpm check (tier: $TIER)"

  # Read check blocks from cpm.toml
  local in_check=false name="" command="" triggers="" severity="" scope=""

  while IFS= read -r line; do
    # New check block
    if [[ "$line" == "[[checks]]" ]]; then
      # Process previous check
      if [[ -n "$name" && -n "$command" ]]; then
        run_single_check
      fi
      name="" command="" triggers="" severity="warning" scope="changed"
      in_check=true
      continue
    fi

    [[ "$in_check" != true ]] && continue

    # Parse fields
    case "$line" in
      name*=*) name=$(echo "$line" | sed 's/.*= *"//;s/".*//') ;;
      command*=*) command=$(echo "$line" | sed 's/.*= *"//;s/".*//') ;;
      triggers*=*) triggers="$line" ;;
      severity*=*) severity=$(echo "$line" | sed 's/.*= *"//;s/".*//') ;;
      scope*=*) scope=$(echo "$line" | sed 's/.*= *"//;s/".*//') ;;
      "["*) in_check=false ;;  # New section, stop
    esac
  done < cpm.toml

  # Process last check
  [[ -n "$name" && -n "$command" ]] && run_single_check

  # Summary
  echo ""
  if [[ $failed -eq 0 ]]; then
    print_summary "${total} checks (${passed} passed, ${skipped} skipped)"
  else
    echo -e "  ${RED}${failed} failed${RESET}, ${passed} passed, ${skipped} skipped (${total} total)"
    if [[ -n "$errors" ]]; then
      echo ""
      echo "$errors"
    fi
    exit 1
  fi
}

run_single_check() {
  total=$((total + 1))

  # Delta detection: skip if triggers don't match
  if [[ "$scope" != "full" ]] && ! triggers_match "$triggers" "$changes"; then
    skipped=$((skipped + 1))
    print_step "" "$name" skip "no matching changes"
    return
  fi

  # Run via wrapper
  if bash "$SCRIPT_DIR/run.sh" "$name" bash "$command" 2>/dev/null; then
    passed=$((passed + 1))
  else
    if [[ "$severity" == "error" ]]; then
      failed=$((failed + 1))
      errors+="  ✗ $name (severity: error)\n"
      [[ "$CPM_RUN_MODE" == "fail-fast" ]] && { echo "$errors"; exit 1; }
    else
      # warning/info — count as passed but show
      passed=$((passed + 1))
    fi
  fi
}

run_checks
