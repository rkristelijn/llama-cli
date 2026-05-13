#!/usr/bin/env bash
#
# check-xref.sh — Validate ADR cross-references in code and docs.
#
# Checks:
# 1. Every "ADR-NNN" mention in src/ has a matching docs/adr/adr-NNN-*.md file
# 2. Every @see docs/adr/... path actually exists
# 3. Every docs/adr/ README link points to an existing file
#
# Usage:
#   bash scripts/lint/check-xref.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
main() {
  print_header "checking cross-references..."
  local errors=0

  # Check 1: ADR-NNN references in source have matching files
  while IFS= read -r line; do
    local file="${line%%:*}"
    local adr_num
    adr_num=$(echo "$line" | sed -n 's/.*ADR-\([0-9]*\).*/\1/p' | head -1)
    if [[ -z "$adr_num" ]]; then
      continue
    fi
    # Pad to 3 digits (strip leading zeros to avoid octal interpretation)
    local padded
    adr_num=$(echo "$adr_num" | sed 's/^0*//')
    padded=$(printf "%03d" "${adr_num:-0}")
    if ! ls docs/adr/adr-"${padded}"-*.md >/dev/null 2>&1; then
      echo "  [warn] ${file}: references ADR-${adr_num} but docs/adr/adr-${padded}-*.md not found"
      ((errors++)) || true
    fi
  done < <(grep -rn "ADR-[0-9]" src/ --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "_test\.\|_it\." || true)

  # Check 2: @see paths exist (skip URLs and same-dir relative refs)
  while IFS= read -r line; do
    local file="${line%%:*}"
    local path
    path=$(echo "$line" | sed -n 's/.*@see \(\S*\).*/\1/p')
    if [[ -n "$path" && "$path" != http* && "$path" == */* && ! -f "$path" ]]; then
      echo "  [warn] ${file}: @see ${path} — file not found"
      ((errors++)) || true
    fi
  done < <(grep -rn "@see " src/ --include="*.cpp" --include="*.h" 2>/dev/null || true)

  # Check 3: ADR README links
  if [[ -f docs/adr/README.md ]]; then
    while IFS= read -r link; do
      if [[ ! -f "docs/adr/${link}" ]]; then
        echo "  [warn] docs/adr/README.md links to ${link} — not found"
        ((errors++)) || true
      fi
    done < <(sed -n 's/.*(\([^)]*\.md\)).*/\1/p' docs/adr/README.md 2>/dev/null || true)
  fi

  if [[ $errors -eq 0 ]]; then
    echo "  ✓ all cross-references valid"
  else
    echo "  [${errors} broken references found]"
  fi
}

main "$@"
