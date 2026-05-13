#!/usr/bin/env bash
#
# Check that comment ratio in src/ meets the minimum threshold.
# Uses cloc CSV output: fields are files,language,blank,comment,code

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
THRESHOLD=20

totals=$(cloc src/ --exclude-dir=test --not-match-f='(_test|_it)\.cpp$' --csv --quiet | grep SUM)
comments=$(echo "$totals" | cut -d',' -f4)
code=$(echo "$totals" | cut -d',' -f5)

total=$((comments + code))
if [ "$total" -eq 0 ]; then
  echo "ERROR: no source lines found in src/"
  exit 1
fi

ratio=$((comments * 100 / total))

echo "Comment ratio: ${comments} comments / ${total} lines = ${ratio}% (minimum: ${THRESHOLD}%)"

if [ "$ratio" -lt "$THRESHOLD" ]; then
  echo "FAIL: comment ratio ${ratio}% is below the ${THRESHOLD}% threshold"
  echo ""
  echo "Per-file breakdown (lowest first):"
  cloc src/ --exclude-dir=test --not-match-f='(_test|_it)\.cpp$' --by-file --csv --quiet |
    grep -v "^$\|^language\|SUM" |
    while IFS=',' read -r _ file _ fc fcode _; do
      ft=$((fc + fcode))
      if [ "$ft" -gt 0 ]; then
        fr=$((fc * 100 / ft))
        printf "  %3d%% (%3d/%4d) %s\n" "$fr" "$fc" "$ft" "$file"
      fi
    done | sort -n | head -15
  echo ""
  echo "Tips:"
  echo "  - Add @file/@brief headers to files missing them"
  echo "  - Add a one-line comment above non-obvious blocks"
  echo "  - When asking AI to write code, include: 'keep comment ratio >= 20%'"
  exit 1
fi
