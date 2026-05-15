#!/usr/bin/env bash
# record.sh — Record a tape with shared defaults prepended.
# Usage: bash docs/demos/record.sh docs/demos/tapes/chat.tape
#        bash docs/demos/record.sh --all
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

DEFAULTS="docs/demos/tapes/_defaults.tape"

record_tape() {
  local tape="$1"
  local tmp
  tmp=$(mktemp /tmp/vhs-tape.XXXXXXXX)
  # Merge: Output line from tape, then defaults, then commands
  grep "^Output " "$tape" > "$tmp"
  cat "$DEFAULTS" >> "$tmp"
  grep -v "^Output \|^#" "$tape" >> "$tmp"
  echo "  [record] $tape"
  # Hide .env to prevent it from overriding mock provider env vars
  if [[ -f .env ]]; then
    mv .env .env.recording-bak
    trap 'mv -f .env.recording-bak .env 2>/dev/null' EXIT
  fi
  vhs "$tmp"
  # Restore .env
  if [[ -f .env.recording-bak ]]; then
    mv .env.recording-bak .env
  fi
  rm -f "$tmp"
}

if [[ "${1:-}" == "--all" ]]; then
  for tape in docs/demos/tapes/*.tape; do
    [[ "$(basename "$tape")" == _* ]] && continue
    record_tape "$tape"
  done
else
  record_tape "${1:?Usage: docs/demos/record.sh <tape|--all>}"
fi
