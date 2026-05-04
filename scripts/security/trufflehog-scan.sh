#!/usr/bin/env bash
#
# trufflehog-scan.sh — Scan for verified secrets using TruffleHog.
#
# Usage:
#   bash scripts/security/trufflehog-scan.sh
#
# TruffleHog verifies secrets against live APIs (AWS, GitHub, etc.)
# complementing gitleaks which uses regex/entropy only.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

cd "$(dirname "$0")/../.."

echo "==> scanning for verified secrets (trufflehog)..."

if ! command -v trufflehog >/dev/null 2>&1; then
  echo "  [skip] trufflehog not installed"
  exit 0
fi

# Scan the filesystem, exclude build dirs and caches
# --exclude-paths expects a file with regex patterns (one per line)
trufflehog filesystem . \
  --no-update \
  --fail \
  --exclude-paths=<(printf '%s\n' '^build/' '^build-fuzz/' '_deps/' '^\.tmp/' '^\.cache/' '^\.git/' 'node_modules/' 'sast\.log' '^\.env') \
  --exclude-detectors=URI

echo "  [done] trufflehog"
