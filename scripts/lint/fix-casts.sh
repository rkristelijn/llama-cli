#!/usr/bin/env bash
#
# fix-casts.sh — Auto-fix C-style casts to C++ casts using clang-tidy

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

print_header "Auto-fixing C-style casts to C++ casts..."

if ! command -v clang-tidy >/dev/null; then
  print_step "" "$(basename "$0" .sh)" skip "clang-tidy not installed"
  exit 0
fi

FIXED=0
for file in src/**/*.cpp src/**/*.h; do
  [[ -f "$file" ]] || continue

  if clang-tidy -fix -checks='-*,cppcoreguidelines-pro-type-cstyle-cast' \
    --format-style=file "$file" -- -std=c++17 -I./src 2>/dev/null; then
    if git diff --quiet "$file" 2>/dev/null; then
      : # No changes
    else
      echo "  [fixed] $file"
      FIXED=$((FIXED + 1))
    fi
  fi
done

echo "  Fixed $FIXED files"
