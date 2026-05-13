#!/usr/bin/env bash
#
# sonar-scan.sh — Run SonarCloud analysis.
#
# SonarCloud analyses run automatically on push via GitHub integration.
# This script is kept for manual local scans if needed.
#
# Requires: SONAR_TOKEN environment variable (generate at https://sonarcloud.io/account/security)
#
# Usage:
#   SONAR_TOKEN=xxx make sonar
#
# Results: https://sonarcloud.io/project/overview?id=rkristelijn_llama-cli

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

# Load token from .env if available
# shellcheck disable=SC1091
[[ -f .env ]] && source .env

if [[ -z "${SONAR_TOKEN:-}" ]]; then
  echo "  ERROR: SONAR_TOKEN not set." >&2
  echo "  Generate one at: https://sonarcloud.io/account/security" >&2
  echo "  Then: export SONAR_TOKEN=your_token" >&2
  exit 1
fi

#######################################
# Ensure sonar-scanner CLI is installed.
#######################################
ensure_scanner() {
  if command -v sonar-scanner >/dev/null 2>&1; then
    return 0
  fi

  echo "  sonar-scanner not found, installing..."
  if [[ "$(uname -s)" == "Darwin" ]]; then
    brew install sonar-scanner
  else
    local ver="6.2.1.4610"
    local url="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${ver}-linux-x64.zip"
    wget -q "${url}" -O /tmp/sonar-scanner.zip
    unzip -qo /tmp/sonar-scanner.zip -d /tmp
    sudo mv "/tmp/sonar-scanner-${ver}-linux-x64" /opt/sonar-scanner
    sudo ln -sf /opt/sonar-scanner/bin/sonar-scanner /usr/local/bin/sonar-scanner
    rm -f /tmp/sonar-scanner.zip
  fi
}

#######################################
# Main entry point.
#######################################
main() {
  echo "==> SonarCloud analysis"

  ensure_scanner

  # Generate compile_commands.json for C++ analysis
  echo "  Generating compilation database..."
  cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null

  # Run scan against SonarCloud (config in sonar-project.properties)
  echo "  Running analysis..."
  sonar-scanner \
    -Dsonar.token="${SONAR_TOKEN}"

  echo ""
  echo "  ✓ Results: https://sonarcloud.io/project/overview?id=rkristelijn_llama-cli"
}

main "$@"
