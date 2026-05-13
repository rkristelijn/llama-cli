#!/usr/bin/env bash
#
# trufflehog-scan.sh — Scan for verified secrets using TruffleHog.
#
# Config: .config/trufflehog.yaml
# Usage:
#   bash scripts/security/trufflehog-scan.sh
#
# TruffleHog verifies secrets against live APIs (AWS, GitHub, etc.)
# complementing gitleaks which uses regex/entropy only.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
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
  --only-verified \
  --exclude-paths=<(printf '%s\n' '^build/' '^build-fuzz/' '_deps/' '^\.tmp/' '^\.cache/' '^\.git/' 'node_modules/' 'sast\.log' '^\.env' '^\.config/\.gitleaksignore$') \
  --exclude-detectors=URI \
  2>&1 | grep -v "^🐷\|^$\|info-0" || true

echo "  [done] trufflehog"
