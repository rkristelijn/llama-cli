#!/usr/bin/env bash
# format-code.sh — Auto-format C++ source files using clang-format.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

echo "==> formatting C++ code..."
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format
echo "  [done] format-code"
