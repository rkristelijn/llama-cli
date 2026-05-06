#!/usr/bin/env bash
#
# fix-braces.sh — Add braces around single-line if/for/while statements

set -o errexit
set -o nounset
set -o pipefail

echo "==> Auto-fixing missing braces..."

FIXED=0

for file in src/**/*.cpp src/**/*.h; do
  [[ -f "$file" ]] || continue
  
  # This is complex - needs proper parsing
  # For now, just detect and report
  if grep -E '^\s*(if|for|while)\s*\([^)]+\)\s*[^{]' "$file" | grep -v '//' | head -1 >/dev/null 2>&1; then
    echo "  [needs-review] $file has single-line statements without braces"
  fi
done

echo ""
echo "  Braces require manual fixes (complex control flow)"
echo "  Use: clang-tidy -checks='readability-braces-around-statements' -fix"
