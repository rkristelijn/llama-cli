#!/usr/bin/env bash
#
# check-licenses.sh — Verify all dependencies have compatible licenses.
#
# Checks CMakeLists.txt FetchContent deps and reports their licenses.
# Fails if any dependency has a copyleft/unknown license.
#
# Usage:
#   bash scripts/lint/check-licenses.sh
#   make licenses
#
# @see docs/adr/adr-048-quality-framework.md (CMMI 2.9)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Known dependencies and their licenses (from CMakeLists.txt FetchContent)
# Update this when adding new deps.
declare -A DEPS=(
  [cpp - httplib]="MIT"
  [doctest]="MIT"
  [dtl]="BSD-3-Clause"
  [cpp_linenoise]="BSD-2-Clause"
)

# Licenses we accept (permissive only)
ALLOWED="MIT BSD-2-Clause BSD-3-Clause Apache-2.0 Zlib ISC Unlicense"

main() {
  echo "==> license check"
  local fail=0

  for dep in "${!DEPS[@]}"; do
    local license="${DEPS[$dep]}"
    if echo "$ALLOWED" | grep -qw "$license"; then
      printf "  ✓ %-20s %s\n" "$dep" "$license"
    else
      printf "  ✗ %-20s %s (NOT ALLOWED)\n" "$dep" "$license"
      fail=$((fail + 1))
    fi
  done

  # Verify deps in CMakeLists.txt match our list
  local cmake_deps
  cmake_deps=$(grep -oP 'FetchContent_Declare\(\s*\K[a-zA-Z_]+' CMakeLists.txt 2>/dev/null || true)
  for d in $cmake_deps; do
    if [[ -z "${DEPS[$d]:-}" ]]; then
      printf "  ✗ %-20s UNKNOWN (add to check-licenses.sh)\n" "$d"
      fail=$((fail + 1))
    fi
  done

  echo ""
  if [[ $fail -gt 0 ]]; then
    echo "  FAIL: $fail license issue(s)"
    exit 1
  fi
  echo "  ✓ all dependencies have permissive licenses"
  echo "  [done] licenses"
}

main "$@"
