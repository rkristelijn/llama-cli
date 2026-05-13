#!/usr/bin/env bash
# format-scripts.sh — Auto-format shell scripts using shfmt.
# @see docs/adr/adr-121-cpm-quality-layer.md (CPM integration)

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
fi

if ! command -v shfmt >/dev/null; then
  print_step 1 "format-scripts" skip "shfmt not installed"
  exit 0
fi

print_header "formatting scripts..."
find scripts e2e -name '*.sh' -exec shfmt -i 2 -w {} \;
