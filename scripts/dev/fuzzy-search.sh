#!/usr/bin/env bash
# Fuzzy search through repo files with smart excludes
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
fd --type f \
  --exclude node_modules \
  --exclude build \
  --exclude build-* \
  --exclude .git \
  --exclude .tmp \
  --exclude .cache \
  --exclude .rumdl_cache |
  fzf --preview 'bat --color=always --style=numbers --line-range=:500 {}'
