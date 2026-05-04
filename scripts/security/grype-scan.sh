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

cd "$(dirname "$0")/../.."

echo "==> scanning for vulnerabilities (grype)..."

if ! command -v grype >/dev/null 2>&1; then
  echo "  [skip] grype not installed"
  exit 0
fi

# Scan project directory using config (don't fail on findings, just report)
grype dir:. -c .config/grype.yaml || true

echo "  [done] grype"
