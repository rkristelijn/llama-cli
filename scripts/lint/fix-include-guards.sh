#!/usr/bin/env bash
#
# fix-include-guards.sh — Standardize include guards to #pragma once

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

echo "==> Converting include guards to #pragma once..."

FIXED=0

for file in src/**/*.h; do
  [[ -f "$file" ]] || continue

  # Check if file has old-style include guards
  if head -5 "$file" | grep -q "#ifndef.*_H" 2>/dev/null; then
    if ! grep -q "#pragma once" "$file" 2>/dev/null; then
      # Has old guard, no pragma once - convert
      # Remove first 2 lines (#ifndef, #define) and last line (#endif)
      sed -i.bak '1,2d; $d' "$file"
      # Add #pragma once at top
      sed -i.bak '1i#pragma once' "$file"
      echo "  [fixed] $file"
      FIXED=$((FIXED + 1))
      rm -f "$file.bak"
    fi
  fi
done

echo "  Fixed $FIXED files"
echo "  Note: Review changes - may need manual adjustment"
