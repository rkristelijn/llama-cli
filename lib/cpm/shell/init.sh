#!/usr/bin/env bash
# init.sh — Single entry point for CPM shell integration.
#
# Scripts source this ONE file. It handles:
#   1. Load ui.sh (colors, print_*, spinner, progress)
#   2. Load timer.sh (timing + trend)
#   3. Load config.sh (cpm.toml → env vars)
#   4. Fallback: if any module is missing, define no-op stubs
#
# Usage (in any script):
#   source lib/cpm/shell/init.sh 2>/dev/null || true
#
# That's it. One line. No inline fallback blocks.
#
# @see docs/adr/adr-121-cpm-quality-layer.md

_CPM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Load modules (order matters: ui first, timer depends on ui)
source "$_CPM_DIR/ui.sh" 2>/dev/null || {
  # Minimal fallback if ui.sh is broken/missing
  print_step()    { echo "  $2 $3${4:+ $4}"; }
  print_header()  { echo "==> $1"; }
  print_error()   { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
  print_summary() { echo "  $1"; }
  spinner_start() { :; }
  spinner_stop()  { :; }
  progress_bar()  { :; }
  timer_start()   { :; }
  timer_stop()    { echo "  $1 $2"; }
}

# Load config (non-fatal if missing)
source "$_CPM_DIR/config.sh" 2>/dev/null || true
