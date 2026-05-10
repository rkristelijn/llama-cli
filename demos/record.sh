#!/usr/bin/env bash
# record.sh — Record a tape with shared defaults prepended.
# Usage: bash demos/record.sh demos/tapes/chat.tape
#        bash demos/record.sh --all
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

DEFAULTS="demos/tapes/_defaults.tape"

record_tape() {
  local tape="$1"
  local tmp
  tmp=$(mktemp /tmp/vhs-tape.XXXXXXXX)
  # Merge: Output line from tape, then defaults, then commands
  grep "^Output " "$tape" > "$tmp"
  cat "$DEFAULTS" >> "$tmp"
  grep -v "^Output \|^#" "$tape" >> "$tmp"
  echo "  [record] $tape"
  vhs "$tmp"
  rm -f "$tmp"
}

if [[ "${1:-}" == "--all" ]]; then
  for tape in demos/tapes/*.tape; do
    [[ "$(basename "$tape")" == _* ]] && continue
    record_tape "$tape"
  done
else
  record_tape "${1:?Usage: demos/record.sh <tape|--all>}"
fi
