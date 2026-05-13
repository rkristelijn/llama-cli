#!/usr/bin/env bash
#
# check-casts.sh — Detect C-style casts (ES.48)

set -o errexit
set -o nounset
set -o pipefail

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi

echo "==> Checking for C-style casts..."

if ! command -v g++ >/dev/null && ! command -v clang++ >/dev/null; then
  echo "  [skip] no C++ compiler found"
  exit 0
fi

CXX="${CXX:-g++}"
FAILED=0

for file in src/**/*.cpp; do
  [[ -f "$file" ]] || continue

  if ! "$CXX" -fsyntax-only -Wold-style-cast -Werror=old-style-cast \
    -I./src -std=c++17 "$file" 2>/dev/null; then
    echo "  [fail] $file has C-style casts"
    FAILED=1
  fi
done

if [[ $FAILED -eq 0 ]]; then
  echo "  [pass] no C-style casts detected"
else
  echo ""
  echo "  C-style casts violate ES.48: Avoid casts"
  echo "  Fix: use static_cast<T>(), dynamic_cast<T>(), or const_cast<T>()"
  exit 1
fi
