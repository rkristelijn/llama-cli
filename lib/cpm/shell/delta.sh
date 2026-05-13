#!/usr/bin/env bash
# delta.sh — Detect changed files and match against trigger patterns.
#
# Functions:
#   cpm_changed_files  — list files changed vs main
#   cpm_triggers_match — check if triggers match any changed file
#
# @see docs/adr/adr-121-cpm-quality-layer.md

# Get changed files vs main (cached per session)
_CPM_CHANGED_CACHE=""
cpm_changed_files() {
  if [[ -z "$_CPM_CHANGED_CACHE" ]]; then
    _CPM_CHANGED_CACHE=$(git diff --name-only main...HEAD 2>/dev/null \
      || git diff --name-only HEAD~1 2>/dev/null \
      || find src scripts -type f 2>/dev/null)
  fi
  echo "$_CPM_CHANGED_CACHE"
}

# Check if trigger patterns match any changed file
# Args: $1 = triggers line from toml, $2 = changed files (newline-separated)
cpm_triggers_match() {
  local triggers="$1"
  local changes="$2"

  while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    [[ "$triggers" == *'"**"'* ]] && return 0
    [[ "$triggers" == *"src/"* && "$file" == src/* ]] && return 0
    [[ "$triggers" == *"scripts/"* && "$file" == scripts/* ]] && return 0
    [[ "$triggers" == *".md"* && "$file" == *.md ]] && return 0
    [[ "$triggers" == *".yml"* && "$file" == *.yml ]] && return 0
    [[ "$triggers" == *".yaml"* && "$file" == *.yaml ]] && return 0
    [[ "$triggers" == *".sh"* && "$file" == *.sh ]] && return 0
    [[ "$triggers" == *"CMakeLists"* && "$file" == *CMakeLists* ]] && return 0
    [[ "$triggers" == *"e2e/"* && "$file" == e2e/* ]] && return 0
    [[ "$triggers" == *"docs/"* && "$file" == docs/* ]] && return 0
  done <<< "$changes"
  return 1
}
