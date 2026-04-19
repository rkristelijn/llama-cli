#!/usr/bin/env bash
# run-e2e.sh — Run all end-to-end tests.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"
BINARY="${2:-$BUILD_DIR/llama-cli}"

if [[ ! -f "$BINARY" ]]; then
  echo "ERROR: binary not found at $BINARY"
  exit 1
fi

echo "==> running e2e tests..."
for t in e2e/*.sh; do
  case "$t" in
    *test_live*|*helpers*) continue ;;
  esac
  
  echo "  [test] $t"
  bash "$t" "$BINARY" > /dev/null || {
    echo "FAIL: $t"
    exit 1
  }
done

echo "  [done] e2e"
