#!/usr/bin/env bash
#
# check-shadowing.sh — Detect variable shadowing (ES.12)

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

echo "==> Checking for variable shadowing..."

if ! command -v g++ >/dev/null && ! command -v clang++ >/dev/null; then
  echo "  [skip] no C++ compiler found"
  exit 0
fi

CXX="${CXX:-g++}"
FAILED=0

for file in src/**/*.cpp src/**/*.h; do
  [[ -f "$file" ]] || continue

  if ! "$CXX" -fsyntax-only -Wshadow -Werror=shadow "$file" 2>/dev/null; then
    echo "  [fail] $file has variable shadowing"
    FAILED=1
  fi
done

if [[ $FAILED -eq 0 ]]; then
  echo "  [pass] no variable shadowing detected"
else
  echo ""
  echo "  Variable shadowing violates ES.12: Do not reuse names in nested scopes"
  echo "  Fix: rename inner variable to avoid shadowing outer scope"
  exit 1
fi
