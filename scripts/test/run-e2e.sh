#!/usr/bin/env bash
# run-e2e.sh — Run all end-to-end tests.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
BUILD_DIR="${1:-build}"
BINARY="${2:-$BUILD_DIR/llama-cli}"

if [[ ! -f "$BINARY" ]]; then
  echo "ERROR: binary not found at $BINARY"
  exit 1
fi

echo "==> running e2e tests..."
# Prevent auto-routing from connecting to real Ollama hosts during tests
export OLLAMA_HOSTS=""
export OLLAMA_HOST="localhost"
export LLAMA_PROVIDER="mock"
# Hide .env to prevent it from overriding mock provider (same as record.sh)
if [[ -f .env ]]; then
  mv .env .env.e2e-bak
  trap 'mv -f .env.e2e-bak .env 2>/dev/null' EXIT
fi
for t in e2e/*.sh; do
  case "$t" in
  *test_live* | *helpers*) continue ;;
  esac

  echo "  [test] $t"
  bash "$t" "$BINARY" >/dev/null || {
    echo "FAIL: $t"
    exit 1
  }
done

echo "  [done] e2e"
