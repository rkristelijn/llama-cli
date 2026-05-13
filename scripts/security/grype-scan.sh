#!/usr/bin/env bash
#
# grype-scan.sh — Scan for vulnerabilities using Grype.
#
# Usage:
#   bash scripts/security/grype-scan.sh
#
# Grype scans the project filesystem for known vulnerabilities
# in dependencies (complements osv-scanner with container support).

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

echo "==> scanning for vulnerabilities (grype)..."

if ! command -v grype >/dev/null 2>&1; then
  echo "  [skip] grype not installed"
  exit 0
fi

# Scan project directory using config (don't fail on findings, just report)
grype dir:. -c .config/grype.yaml || true

echo "  [done] grype"
