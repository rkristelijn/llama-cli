#!/usr/bin/env bash
#
# trivi.sh — Analyze repository for "implicit decisions" (trivi).
# Scans for HACKs, architecture markers, and unlogged decisions.
#
# Usage:
#   bash scripts/dev/trivi.sh
#
# @see docs/adr/adr-047-ai-guided-development-qa.md

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
main() {
  echo "==> Scanning for implicit decisions (TRIVI)..."

  # Search for markers of implicit architectural or design decisions
  # HACK: Fast fix, might be technical debt
  # FIXME: Known bug/limitation
  # NOTE/IMP: Potentially implicit decision
  local findings
  findings=$(grep -rnE '\b(HACK|FIXME|IMP|DECISION)\b' src/ scripts/ --include="*.cpp" --include="*.h" --include="*.sh" 2>/dev/null || true)

  if [[ -z "${findings}" ]]; then
    echo "No implicit decision markers found."
  else
    echo "${findings}"
  fi

  # Report to stdout for TUI
  echo ""
  echo "==> Recommendation"
  echo "If any of these HACKs or FIXMEs have architectural implications,"
  echo "please document them in an ADR (docs/adr/)."
}

main "$@"
