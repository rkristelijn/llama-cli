#!/usr/bin/env bash
#
# check-deps.sh — Verify required and optional tools are installed.
#
# Required tools cause a hard fail. Optional tools show a warning.
#
# Usage:
#   bash scripts/check/check-deps.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

REQUIRED=(cmake clang-format clang-tidy cppcheck pmccabe cloc doxygen)
OPTIONAL=(semgrep gitleaks shellcheck yamllint rumdl)

missing=0
warnings=0

main() {
  echo "==> Checking required tools..."
  for tool in "${REQUIRED[@]}"; do
    if command -v "${tool}" >/dev/null 2>&1; then
      printf "  %-20s ✓\n" "${tool}"
    else
      printf "  %-20s MISSING\n" "${tool}"
      (( missing++ )) || true
    fi
  done

  echo ""
  echo "==> Checking optional tools..."
  for tool in "${OPTIONAL[@]}"; do
    if command -v "${tool}" >/dev/null 2>&1; then
      printf "  %-20s ✓\n" "${tool}"
    else
      printf "  %-20s not installed (optional)\n" "${tool}"
      (( warnings++ )) || true
    fi
  done

  echo ""
  if (( missing > 0 )); then
    echo "${missing} required tool(s) missing. Run 'make setup' to install."
    exit 1
  fi
  if (( warnings > 0 )); then
    echo "All required tools present. ${warnings} optional tool(s) missing."
  else
    echo "All tools present."
  fi
}

main "$@"
