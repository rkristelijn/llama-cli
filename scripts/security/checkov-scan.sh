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

echo "==> scanning IaC configs (checkov)..."

if ! command -v checkov >/dev/null 2>&1; then
  echo "  [skip] checkov not installed"
  exit 0
fi

# Run checkov with config file
checkov --config-file .config/checkov.yaml

echo "  [done] checkov"
