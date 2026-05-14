#!/usr/bin/env bash
#
# osv-scan.sh — Scan for known vulnerabilities using Google OSV.
#
# Usage:
#   bash scripts/security/osv-scan.sh
#
# OSV-Scanner uses the Google OSV database for accurate
# vulnerability detection in project dependencies.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
cd "$(dirname "$0")/../.."

echo "==> scanning for vulnerabilities (osv-scanner)..."

if ! command -v osv-scanner >/dev/null 2>&1; then
  echo "  [skip] osv-scanner not installed"
  exit 0
fi

# Scan project root, skip build artifacts
# osv-scanner v2 uses 'scan' subcommand with --config for ignore rules
osv-scanner scan --config .config/osv-scanner.toml -r \
  --experimental-exclude build \
  --experimental-exclude build-fuzz \
  --experimental-exclude .tmp \
  --experimental-exclude .cache \
  . || true

echo "  [done] osv-scanner"
