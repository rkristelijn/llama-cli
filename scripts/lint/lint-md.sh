#!/usr/bin/env bash
# lint-md.sh — Run rumdl checks on Markdown files.
# Config: .config/rumdl.toml

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
if ! command -v rumdl >/dev/null; then
  echo "  [skip] rumdl not installed"
  exit 0
fi

echo "==> linting markdown..."
rumdl check .
echo "  [done] lint-md"
