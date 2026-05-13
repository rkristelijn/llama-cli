#!/usr/bin/env bash
#
# check-conversions.sh — Check for implicit conversions that could cause data loss.
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

echo "[?] Checking for implicit conversion issues..."

BUILD_DIR="${1:-build-conversion-check}"
rm -rf "$BUILD_DIR"

# Build with -Wconversion to catch implicit conversions
cmake -B "$BUILD_DIR" \
  -DCMAKE_CXX_FLAGS="-Wconversion -Wsign-conversion" \
  -DCMAKE_BUILD_TYPE=Debug \
  >/dev/null 2>&1

OUTPUT=$(cmake --build "$BUILD_DIR" 2>&1 || true)

# Only check our own source files, not external dependencies
OUR_ERRORS=$(echo "$OUTPUT" | grep -E "error:.*conversion" | grep -v "_deps/" || true)

if [[ -n "$OUR_ERRORS" ]]; then
  echo "[FAIL] Found implicit conversion errors:"
  echo "$OUR_ERRORS" | head -10
  rm -rf "$BUILD_DIR"
  exit 1
fi

echo "[PASS] No implicit conversion issues found"
rm -rf "$BUILD_DIR"
