#!/usr/bin/env bash
#
# syft-sbom.sh — Generate Software Bill of Materials using Syft.
#
# Usage:
#   bash scripts/security/syft-sbom.sh
#
# Syft generates an SBOM that can be fed into grype or other
# vulnerability scanners for accurate dependency tracking.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

cd "$(dirname "$0")/../.."

echo "==> generating SBOM (syft)..."

if ! command -v syft >/dev/null 2>&1; then
  echo "  [skip] syft not installed"
  exit 0
fi

# Generate SBOM for the project directory
syft dir:. --exclude='./build/**' --exclude='./build-fuzz/**' --exclude='./_deps/**' -o table

echo "  [done] syft"
