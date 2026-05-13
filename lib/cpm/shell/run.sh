#!/usr/bin/env bash
# run.sh — Universal wrapper: timing, live output, logging.
#
# Usage:
#   bash lib/cpm/shell/run.sh <name> <command> [args...]
#
# Output is LIVE (streamed to console) AND logged to file simultaneously.
#
# @see docs/adr/adr-121-cpm-quality-layer.md

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ui.sh"

name="$1"
shift

CPM_OUTPUT="${CPM_OUTPUT:-both}"
CPM_LOG_DIR="${CPM_LOG_DIR:-.tmp}"
mkdir -p "$CPM_LOG_DIR"
logfile="$CPM_LOG_DIR/${name}.log"

timer_start "$name"

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
    # Live output: tee to file AND console simultaneously
    # Use pipe with PIPESTATUS to get real exit code
    set +o pipefail
    "$@" 2>&1 | tee "$logfile"
    rc=${PIPESTATUS[0]}
    set -o pipefail
    ;;
esac

if ((rc == 0)); then
  timer_stop "$name" success
else
  timer_stop "$name" error
  exit $rc
fi
