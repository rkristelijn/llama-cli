#!/usr/bin/env bash
#
# check-theme.sh — Enforce that ANSI color codes only appear in src/tui/.
#
# Any hardcoded \033[ outside the TUI package is a violation of ADR-080.
# Exceptions: test files, the theme.h itself.
#
# Usage:
#   bash scripts/lint/check-theme.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  echo "==> checking theme compliance (ADR-080)..."

  # Find hardcoded ANSI escape codes outside src/tui/
  local violations
  violations=$(grep -rn '\\033\[' src/ --include="*.cpp" --include="*.h" \
    | grep -v "src/tui/" \
    | grep -v "src/trace/" \
    | grep -v "_test\." \
    | grep -v "_it\." \
    || true)

  if [[ -n "${violations}" ]]; then
    echo "  [WARN] Hardcoded ANSI colors found outside src/tui/ (ADR-080 violation):"
    echo "${violations}" | sed 's/^/    /'
    echo ""
    echo "  Fix: use tui::active_theme().<role>.ansi() + Style::reset() instead."
    echo "  See docs/adr/adr-080-theme-system.md"
    # Warning only for now — change to exit 1 once migration is complete
    echo "  [done] check-theme (warnings only)"
  else
    echo "  ✓ no hardcoded ANSI codes outside src/tui/"
    echo "  [done] check-theme"
  fi
}

main "$@"
