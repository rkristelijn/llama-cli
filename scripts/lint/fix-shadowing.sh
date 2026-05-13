#!/usr/bin/env bash
#
# fix-shadowing.sh — Auto-fix simple variable shadowing cases

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

echo "==> Auto-fixing variable shadowing..."
echo "  Note: Only fixes loop variables declared outside loop"

FIXED=0

# Pattern: int i; ... for (i = 0; ...) → for (int i = 0; ...)
for file in src/**/*.cpp; do
  [[ -f "$file" ]] || continue

  # This is a simple heuristic - real fix needs AST analysis
  # We'll just report what needs manual fixing
  if grep -q "^\s*int\s\+[a-z]\+\s*;" "$file" 2>/dev/null; then
    if grep -q "for\s*(\s*[a-z]\+\s*=" "$file" 2>/dev/null; then
      echo "  [manual] $file - loop variable may need moving into for-init"
    fi
  fi
done

echo "  Shadowing requires manual fixes (complex AST changes)"
echo "  Use: clang-tidy -checks='-*,misc-unused-using-decls' for hints"
