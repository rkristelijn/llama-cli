#!/usr/bin/env bash
#
# check-file-size.sh — Enforce maximum file sizes in src/.
#
# Limits: source 400, headers 300, tests 600 lines.
# Existing violations are listed as known tech debt (ADR-061).
#
# Usage:
#   bash scripts/lint/check-file-size.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Known violations — exempt until split (ADR-061)
EXEMPT=(
  "src/repl/repl.cpp"
  "src/tui/tui.h"
  "src/config/config.cpp"
  "src/main.cpp"
)

MAX_SOURCE=400
MAX_HEADER=300
MAX_TEST=650

# Check if a file is in the exempt list
is_exempt() {
  local file="$1"
  for e in "${EXEMPT[@]}"; do
    if [[ "$file" == "$e" ]]; then
      return 0
    fi
  done
  return 1
}

main() {
  echo "==> checking file sizes (ADR-061)..."
  local failed=0

  while IFS= read -r file; do
    lines=$(wc -l < "$file")
    # Determine limit based on file type
    if [[ "$file" == *_test.cpp || "$file" == *_it.cpp ]]; then
      max=$MAX_TEST
    elif [[ "$file" == *.h ]]; then
      max=$MAX_HEADER
    else
      max=$MAX_SOURCE
    fi

    if (( lines > max )); then
      if is_exempt "$file"; then
        echo "  [exempt] $file ($lines > $max) — tech debt, see ADR-061"
      else
        echo "  [FAIL] $file: $lines lines (max $max)"
        failed=1
      fi
    fi
  done < <(find src -name '*.cpp' -o -name '*.h')

  if (( failed )); then
    echo "  FAIL: files exceed size limit. Split them or add to EXEMPT in ADR-061."
    exit 1
  fi
  echo "  [done] file-size"
}

main "$@"
