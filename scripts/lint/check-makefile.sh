#!/usr/bin/env bash
#
# lint-makefile.sh — Check that Makefile targets don't have too much inline shell.
#
# Targets with >5 physical shell lines (excluding @echo and $(MAKE) calls)
# should be extracted to scripts.
#
# Usage:
#   bash scripts/check/lint-makefile.sh [--max N] [Makefile]
#
# Environment:
#   TRACE=1    Enable debug tracing
#
# @see docs/adr/adr-44-tidy-boilerplate.md — extraction rule
# @see docs/tools/shell-scripts.md — shell conventions

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MAX_LINES=5
MAKEFILE="Makefile"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --max) MAX_LINES="$2"; shift 2 ;;
    *)     MAKEFILE="$1"; shift ;;
  esac
done

#######################################
# Count physical shell lines per target.
# Excludes @echo and $(MAKE) lines.
#######################################
main() {
  local target=""
  local count=0
  local failures=0

  while IFS= read -r line; do
    # New target (not a variable assignment)
    if [[ "${line}" =~ ^[a-zA-Z_][a-zA-Z0-9_-]*:.* ]] && [[ ! "${line}" =~ ^[A-Z_]+[[:space:]]*[\?:]?= ]]; then
      if [[ -n "${target}" ]] && (( count > MAX_LINES )); then
        printf "  %-25s %d lines (max %d)\n" "${target}" "${count}" "${MAX_LINES}"
        (( failures++ )) || true
      fi
      target="${line%%:*}"
      count=0
      continue
    fi

    # Recipe line (tab-indented)
    if [[ "${line}" =~ ^$'\t' ]] && [[ -n "${target}" ]]; then
      local stripped="${line#$'\t'}"
      stripped="${stripped#@}"

      # Skip echo-only lines
      [[ "${stripped}" =~ ^echo[[:space:]] ]] && continue
      # Skip $(MAKE) delegation
      [[ "${stripped}" =~ ^\$\(MAKE\) ]] && continue

      (( count++ )) || true
    elif [[ -z "${line}" ]]; then
      : # blank line inside target — ignore
    else
      # Non-recipe, non-blank — end of target
      if [[ -n "${target}" ]] && (( count > MAX_LINES )); then
        printf "  %-25s %d lines (max %d)\n" "${target}" "${count}" "${MAX_LINES}"
        (( failures++ )) || true
      fi
      target=""
      count=0
    fi
  done < "${MAKEFILE}"

  # Last target
  if [[ -n "${target}" ]] && (( count > MAX_LINES )); then
    printf "  %-25s %d lines (max %d)\n" "${target}" "${count}" "${MAX_LINES}"
    (( failures++ )) || true
  fi

  if (( failures > 0 )); then
    echo ""
    echo "  ${failures} target(s) exceed ${MAX_LINES}-line limit. Extract to scripts/."
    return 1
  fi
}

main "$@"
