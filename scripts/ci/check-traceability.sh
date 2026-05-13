#!/usr/bin/env bash
#
# check-traceability.sh — Bidirectional traceability check (ADR-095).
#
# Enforces three invariants:
#   1. Every LOG_FEATURE token in src/ exists in feature-registry.yml
#   2. Every ADR referenced in feature-registry.yml exists on disk
#   3. Every ADR with status "Implemented" is referenced in the registry (warning only)
#
# Usage:
#   bash scripts/ci/check-traceability.sh

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
REGISTRY="docs/feature-registry.yml"
ADR_DIR="docs/adr"
FAIL=0

# --- Check 1: Every LOG_FEATURE in src/ is in the registry ---
echo "==> Check 1: Code → Registry (every LOG_FEATURE token is registered)"
code_tokens=$(grep -roh 'LOG_FEATURE("[^"]*")' src/ | sed 's/LOG_FEATURE("//;s/")//' | sort -u)
registry_tokens=$(grep -E '^[a-z_]+:' "$REGISTRY" | sed 's/:.*//' | sort -u)

missing_from_registry=()
for token in $code_tokens; do
  if ! echo "$registry_tokens" | grep -qx "$token"; then
    missing_from_registry+=("$token")
  fi
done

if [ ${#missing_from_registry[@]} -gt 0 ]; then
  echo "  FAIL: ${#missing_from_registry[@]} token(s) in src/ not in registry:"
  printf '    %s\n' "${missing_from_registry[@]}"
  FAIL=1
else
  echo "  PASS: all $(echo "$code_tokens" | wc -l | tr -d ' ') tokens registered"
fi

# --- Check 2: Every ADR in registry exists on disk ---
echo ""
echo "==> Check 2: Registry → Docs (every referenced ADR exists)"
registry_adrs=$(grep -oE 'ADR-[0-9]+' "$REGISTRY" | sort -u)

missing_adrs=()
for adr in $registry_adrs; do
  num=$(echo "$adr" | sed 's/ADR-//')
  if ! ls "$ADR_DIR"/adr-${num}-*.md >/dev/null 2>&1; then
    missing_adrs+=("$adr")
  fi
done

if [ ${#missing_adrs[@]} -gt 0 ]; then
  echo "  FAIL: ${#missing_adrs[@]} ADR(s) referenced but not found:"
  printf '    %s\n' "${missing_adrs[@]}"
  FAIL=1
else
  echo "  PASS: all $(echo "$registry_adrs" | wc -l | tr -d ' ') referenced ADRs exist"
fi

# --- Check 3: Every "Implemented" ADR appears in registry (warning) ---
echo ""
echo "==> Check 3: Docs → Registry (implemented ADRs have feature proof)"
implemented_adrs=$(grep -l "Implemented" "$ADR_DIR"/adr-*.md | sed 's|.*/adr-||;s|-.*||' | sort -u)

unlinked=()
for num in $implemented_adrs; do
  adr_ref="ADR-${num}"
  if ! grep -q "$adr_ref" "$REGISTRY" 2>/dev/null; then
    unlinked+=("$adr_ref")
  fi
done

if [ ${#unlinked[@]} -gt 0 ]; then
  echo "  WARN: ${#unlinked[@]} implemented ADR(s) not referenced in registry:"
  printf '    %s\n' "${unlinked[@]}"
  echo "  (These may be process/infra ADRs without LOG_FEATURE tokens — not a failure)"
else
  echo "  PASS: all implemented ADRs referenced"
fi

# --- Summary ---
echo ""
if [ "$FAIL" -gt 0 ]; then
  echo "FAIL: traceability check failed"
  exit 1
fi
echo "PASS: bidirectional traceability verified (ADR-095)"
