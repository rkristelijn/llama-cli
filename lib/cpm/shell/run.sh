#!/usr/bin/env bash
# run.sh — Universal wrapper: timing, output routing, trend analysis.
#
# Usage (from Makefile):
#   @bash lib/cpm/shell/run.sh "check-name" command args...
#
# Output modes (CPM_OUTPUT): both | console | file
# Run modes (CPM_RUN_MODE): collect | fail-fast
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

# Run command and capture exit code reliably
case "$CPM_OUTPUT" in
  file)
    "$@" > "$logfile" 2>&1
    rc=$?
    ;;
  console)
    "$@"
    rc=$?
    ;;
  *)  # both (default)
    # Use temp file to avoid PIPESTATUS issues with tee
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
