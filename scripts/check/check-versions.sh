#!/usr/bin/env bash
#
# check-versions.sh — Compare installed tool versions against .config/versions.env.
#
# Usage:
#   bash scripts/check/check-versions.sh
#
# @see .config/versions.env
# @see docs/adr/adr-026-version-pinning.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# shellcheck source=../../.config/versions.env
source .config/versions.env

ok=0
fail=0

check() {
  local name="$1" expected="$2" actual="$3"
  if [[ -z "${actual}" ]]; then
    printf "  %-20s MISSING (expected %s)\n" "${name}" "${expected}"
    (( fail++ )) || true
  elif echo "${actual}" | grep -q "${expected}"; then
    printf "  %-20s %s ✓\n" "${name}" "${actual}"
    (( ok++ )) || true
  else
    printf "  %-20s %s (expected %s)\n" "${name}" "${actual}" "${expected}"
    (( fail++ )) || true
  fi
}

main() {
  echo "==> Checking tool versions against .config/versions.env"
  check "cmake"        "${CMAKE_VERSION}"      "$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"
  check "clang-format" "${LLVM_VERSION}"        "$(clang-format --version 2>/dev/null | grep -oE '[0-9]+' | head -1)"
  check "clang-tidy"   "${LLVM_VERSION}"        "$(clang-tidy --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1 | cut -d. -f1)"
  check "cppcheck"     "${CPPCHECK_VERSION}"    "$(cppcheck --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+')"
  check "doxygen"      "${DOXYGEN_VERSION}"     "$(doxygen --version 2>/dev/null)"
  check "cloc"         "${CLOC_VERSION}"        "$(cloc --version 2>/dev/null)"
  check "shellcheck"   "${SHELLCHECK_VERSION}"  "$(shellcheck --version 2>/dev/null | grep '^version:' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
  check "yamllint"     "${YAMLLINT_VERSION}"    "$(yamllint --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
  check "rumdl"        "${RUMDL_VERSION}"       "$(rumdl version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
  echo ""
  echo "  ${ok} OK, ${fail} mismatch/missing"
  [[ "${fail}" -eq 0 ]] || echo "  Run 'make setup' to install missing tools."
}

main "$@"
