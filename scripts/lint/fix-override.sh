#!/usr/bin/env bash
#
# fix-override.sh — Auto-add override keyword to virtual functions

set -o errexit
set -o nounset
set -o pipefail

echo "==> Auto-fixing missing override keywords..."

# First check if we have any virtual functions that need override
NEEDS_FIX=0
for file in src/**/*.h src/**/*.cpp; do
  [[ -f "$file" ]] || continue
  
  # Simple heuristic: look for virtual functions in derived classes
  if grep -q "virtual.*(" "$file" 2>/dev/null; then
    if ! grep -q "override" "$file" 2>/dev/null; then
      NEEDS_FIX=1
      break
    fi
  fi
done

if [[ $NEEDS_FIX -eq 0 ]]; then
  echo "  [pass] All virtual functions already have override"
  exit 0
fi

# Use sed to add override to virtual functions
FIXED=0
for file in src/**/*.h src/**/*.cpp; do
  [[ -f "$file" ]] || continue
  
  # Pattern: virtual TYPE func(...) { or virtual TYPE func(...);
  # Add override before { or ;
  if sed -i.bak -E 's/(virtual[^;{]+)(\s*[;{])/\1 override\2/g' "$file" 2>/dev/null; then
    if ! diff -q "$file" "$file.bak" >/dev/null 2>&1; then
      echo "  [fixed] $file"
      FIXED=$((FIXED + 1))
    fi
    rm -f "$file.bak"
  fi
done

echo "  Fixed $FIXED files"
echo "  Note: Review changes - may need manual cleanup"
