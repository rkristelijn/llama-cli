#!/usr/bin/env bash
#
# check-unicode.sh — Check for invisible Unicode characters that could be security backdoors.
set -o errexit
set -o nounset
set -o pipefail

echo "[?] Checking for invisible Unicode characters..."

FOUND=0

# Check for common invisible characters:
# U+200B ZERO WIDTH SPACE
# U+200C ZERO WIDTH NON-JOINER  
# U+200D ZERO WIDTH JOINER
# U+FEFF ZERO WIDTH NO-BREAK SPACE (BOM)

find src/ -type f \( -name "*.cpp" -o -name "*.h" \) | while read -r file; do
  if grep -qP '[\x{200B}\x{200C}\x{200D}\x{FEFF}]' "$file" 2>/dev/null; then
    echo "[FAIL] Invisible Unicode character found in: $file"
    FOUND=1
  fi
done

if [ "$FOUND" -eq 1 ]; then
  echo "[FAIL] Found invisible Unicode characters (potential backdoor)"
  exit 1
fi

echo "[PASS] No invisible Unicode characters found"
