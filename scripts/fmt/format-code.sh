#!/usr/bin/env bash
# format-code.sh — Auto-format C++ source files using clang-format.
# @see docs/adr/adr-121-cpm-quality-layer.md

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

print_header "formatting C++ code..."
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format
