#!/usr/bin/env bash
#
# prepush.sh — Run appropriate checks based on what changed vs main.
#
# If code changed: run full check. If only docs: run index check.
#
# Usage:
#   bash scripts/dev/prepush.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  local changed
  changed="$(git diff --name-only origin/main...HEAD)"

  if echo "${changed}" | grep -qE '\.(cpp|h)$'; then
    echo "==> make check (code changed)"
    make -s check
  elif echo "${changed}" | grep -qvE '\.(md|rst)$'; then
    echo "==> make check (non-doc files changed)"
    make -s check
  else
    echo "==> make index (docs only)"
    make -s index
    git diff --quiet INDEX.md || { echo "FAIL: INDEX.md outdated"; exit 1; }
    echo "All checks passed."
  fi
}

main "$@"
