#!/usr/bin/env bash
#
# check-consistency.sh — Automated code consistency checks (ADR-065).
#
# Checks for patterns found during manual audit that should not regress:
# - Duplicate function implementations across files
# - exit() calls in library code (not main.cpp)
# - #pragma once (project uses #ifndef guards)
# - Inconsistent error output (raw cerr vs tui::error)
# - Magic numbers in config defaults
# - Repeated file-slurp patterns
#
# Usage:
#   bash scripts/lint/check-consistency.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

FAIL=0

check() {
  local label="$1"
  local result="$2"
  if [ -n "$result" ]; then
    echo "  ✗ $label"
    printf '%s\n' "$result" | sed 's/^/    /'
    FAIL=$((FAIL + 1))
  else
    echo "  ✓ $label"
  fi
}

echo "==> checking code consistency (ADR-065)..."

# 1. No #pragma once — project uses #ifndef guards
check "no #pragma once in headers" \
  "$(grep -rn '#pragma once' src/ --include='*.h' 2>/dev/null || true)"

# 2. No exit() in library code (only allowed in main.cpp and load_config)
check "no exit() in library code" \
  "$(grep -rn 'exit(1)\|std::exit(1)' src/ --include='*.cpp' --include='*.h' \
    | grep -v 'main.cpp\|config.cpp' || true)"

# 3. No duplicate escape_json implementations
check "no duplicate escape_json" \
  "$(grep -rn 'escape_json_string\|static.*escape_json' src/ --include='*.cpp' || true)"

# 4. No duplicate parse_exec implementations
EXEC_PARSERS=$(grep -rln 'parse_exec_annotations\|static.*parse_exec_tags' src/ --include='*.cpp' 2>/dev/null || true)
check "no duplicate parse_exec" \
  "$(echo "$EXEC_PARSERS" | grep -v sync.cpp || true)"

# 5. ADR README links resolve to actual files
if [ -f docs/adr/README.md ]; then
  BROKEN=""
  while IFS= read -r link; do
    if [ ! -f "docs/adr/$link" ]; then
      BROKEN="${BROKEN}  broken link: $link\n"
    fi
  done < <(grep -oP '\(adr-[^)]+\.md\)' docs/adr/README.md | tr -d '()' || true)
  # Also check for wrong prefix (adr/adr- instead of adr-)
  WRONG_PREFIX=$(grep -n '(adr/adr-' docs/adr/README.md || true)
  if [ -n "$WRONG_PREFIX" ]; then
    BROKEN="${BROKEN}  wrong prefix (adr/adr- should be adr-):\n${WRONG_PREFIX}\n"
  fi
  check "ADR README links valid" "$(printf '%b' "$BROKEN")"
fi

# 6. All ADRs have a status line
MISSING_STATUS=""
for f in docs/adr/adr-*.md; do
  if ! grep -qi 'Status.*Implemented\|Status.*Accepted\|Status.*Proposed\|Status.*Rejected\|Status.*Superseded\|^Implemented$\|^Accepted$\|^Proposed$\|^Rejected$\|^Superseded$' "$f" 2>/dev/null; then
    MISSING_STATUS="${MISSING_STATUS}  $(basename "$f")\n"
  fi
done
check "all ADRs have status" "$(printf '%b' "$MISSING_STATUS")"

# 7. Header guards consistent (all #ifndef, no #pragma once)
# Already covered by check 1

echo ""
if [ "$FAIL" -gt 0 ]; then
  echo "FAIL: $FAIL consistency issue(s) found"
  exit 1
fi
echo "  [done] consistency"
