#!/usr/bin/env bash
# format-code.sh — Auto-format C++ source files using clang-format.
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true

print_header "formatting C++ code..."
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format
