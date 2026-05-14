#!/usr/bin/env bash
#
# checkov-scan.sh — Scan IaC and configs for misconfigurations using Checkov.
#
# Usage:
#   bash scripts/security/checkov-scan.sh
#
# Checkov scans Dockerfiles, GitHub Actions, and YAML configs
# for security misconfigurations and policy violations.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
cd "$(dirname "$0")/../.."

echo "==> scanning IaC configs (checkov)..."

if ! command -v checkov >/dev/null 2>&1; then
  echo "  [skip] checkov not installed"
  exit 0
fi

# Run checkov with config file
checkov --config-file .config/checkov.yaml

echo "  [done] checkov"
