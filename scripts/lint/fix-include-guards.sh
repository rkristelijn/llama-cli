#!/usr/bin/env bash
#
# fix-include-guards.sh — Standardize include guards to #pragma once

set -o errexit
set -o nounset
set -o pipefail

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
