#!/usr/bin/env bash
# run.sh — Universal wrapper: timing, delta detection, output routing.
#
# Usage:
#   bash lib/cpm/shell/run.sh <name> <command> [args...]
#
# Environment:
#   CPM_OUTPUT    — both | console | file (default: both)
#   CPM_SCOPE    — override scope: full | changed | diff
#   CPM_CHANGED  — pre-computed changed files (newline-separated)
#
# @see docs/adr/adr-121-cpm-quality-layer.md

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ui.sh"

name="$1"
shift

# Output routing
CPM_OUTPUT="${CPM_OUTPUT:-both}"
CPM_LOG_DIR="${CPM_LOG_DIR:-.tmp}"
mkdir -p "$CPM_LOG_DIR"
logfile="$CPM_LOG_DIR/${name}.log"

timer_start "$name"

# Run command, capture exit code
case "$CPM_OUTPUT" in
  file)
    "$@" > "$logfile" 2>&1
    rc=$?
    ;;
  console)
    "$@"
    rc=$?
    ;;
  *)
    "$@" > "$logfile" 2>&1
    rc=$?
    cat "$logfile"
    ;;
esac

if ((rc == 0)); then
  timer_stop "$name" success
else
  timer_stop "$name" error
  exit $rc
fi
