#!/usr/bin/env bash
#
# fix-nullptr.sh — Auto-fix NULL/0 to nullptr using sed

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

echo "==> Auto-fixing NULL to nullptr..."

FIXED=0

for file in src/**/*.cpp src/**/*.h; do
  [[ -f "$file" ]] || continue

  # Replace NULL with nullptr (word boundary)
  if sed -i.bak 's/\bNULL\b/nullptr/g' "$file" 2>/dev/null; then
    if ! diff -q "$file" "$file.bak" >/dev/null 2>&1; then
      echo "  [fixed] $file"
      FIXED=$((FIXED + 1))
    fi
    rm -f "$file.bak"
  fi
done

echo "  Fixed $FIXED files (NULL → nullptr)"
echo "  Note: 0 → nullptr requires manual review (may be integer 0)"
