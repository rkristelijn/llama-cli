#!/usr/bin/env bash
#
# fix-casts.sh — Auto-fix C-style casts to C++ casts using clang-tidy

set -o errexit
set -o nounset
set -o pipefail

echo "==> Auto-fixing C-style casts to C++ casts..."

if ! command -v clang-tidy >/dev/null; then
  echo "  [skip] clang-tidy not installed"
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
