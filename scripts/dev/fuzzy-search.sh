#!/usr/bin/env bash
# Fuzzy search through repo files with smart excludes

fd --type f \
  --exclude node_modules \
  --exclude build \
  --exclude build-* \
  --exclude .git \
  --exclude .tmp \
  --exclude .cache \
  --exclude .rumdl_cache \
  | fzf --preview 'bat --color=always --style=numbers --line-range=:500 {}'
