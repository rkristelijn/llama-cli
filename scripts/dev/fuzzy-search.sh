#!/usr/bin/env bash
# Fuzzy search through repo files with smart excludes
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
fd --type f \
  --exclude node_modules \
  --exclude build \
  --exclude build-* \
  --exclude .git \
  --exclude .tmp \
  --exclude .cache \
  --exclude .rumdl_cache |
  fzf --preview 'bat --color=always --style=numbers --line-range=:500 {}'
