#!/usr/bin/env bash
#
# codeql-scan.sh — Deep SAST analysis using CodeQL.
#
# Usage:
#   bash scripts/security/codeql-scan.sh
#
# CodeQL performs deep data-flow and taint analysis.
# Requires codeql CLI (GitHub). Slower than semgrep but finds
# real vulnerabilities with low false-positive rate.

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

echo "==> running deep SAST (codeql)..."

if ! command -v codeql >/dev/null 2>&1; then
  echo "  [skip] codeql not installed (install via gh extension or GitHub releases)"
  exit 0
fi

DB_DIR=".tmp/codeql-db"

# Create/update the database
echo "  creating codeql database..."
codeql database create "${DB_DIR}" --language=cpp --overwrite --source-root=src 2>&1 | tail -5

# Run analysis
echo "  analyzing..."
codeql database analyze "${DB_DIR}" --format=sarif-latest --output=.tmp/codeql-results.sarif \
  codeql/cpp-queries:codeql-suites/cpp-security-and-quality.qls 2>&1 | tail -5

# Check for results
if [[ -f .tmp/codeql-results.sarif ]]; then
  FINDINGS=$(grep -c '"level"' .tmp/codeql-results.sarif 2>/dev/null || echo "0")
  echo "  findings: ${FINDINGS}"
fi

echo "  [done] codeql"
