#!/usr/bin/env bash
# format-scripts.sh — Auto-format shell scripts using shfmt.
# @see docs/adr/adr-121-cpm-quality-layer.md (CPM integration)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true

if ! command -v shfmt >/dev/null; then
  print_step 1 "format-scripts" skip "shfmt not installed"
  exit 0
fi

print_header "formatting scripts..."
find scripts e2e -name '*.sh' -exec shfmt -i 2 -w {} \; 2>/dev/null || find scripts -name '*.sh' -exec shfmt -i 2 -w {} \;
