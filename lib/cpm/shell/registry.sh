#!/usr/bin/env bash
# registry.sh — Parse [[checks]] from cpm.toml into callable data.
#
# Functions:
#   cpm_parse_checks — yields check records via callback
#
# @see docs/adr/adr-121-cpm-quality-layer.md

# Parse checks from cpm.toml, call handler for each check.
# Handler receives: name command triggers scope severity
cpm_parse_checks() {
  local handler="$1"
  local config="${2:-cpm.toml}"

  [[ -f "$config" ]] || return 1

  local in_checks=false
  local name="" command="" triggers="" scope="changed" severity="warning"

  while IFS= read -r line; do
    if [[ "$line" == "[[checks]]" ]]; then
      # Emit previous check
      [[ -n "$name" && -n "$command" ]] && $handler "$name" "$command" "$triggers" "$scope" "$severity"
      name="" command="" triggers="" scope="changed" severity="warning"
      in_checks=true
      continue
    fi

    # Non-checks section ends parsing
    [[ "$line" == "["* && "$line" != "[[checks]]" ]] && {
      [[ -n "$name" && -n "$command" ]] && $handler "$name" "$command" "$triggers" "$scope" "$severity"
      name="" command=""
      in_checks=false
      continue
    }

    [[ "$in_checks" != true ]] && continue

    # Parse fields
    case "$line" in
      name*=*) name="${line#*= \"}"; name="${name%\"}" ;;
      command*=*) command="${line#*= \"}"; command="${command%\"}" ;;
      triggers*=*) triggers="$line" ;;
      scope*=*) scope="${line#*= \"}"; scope="${scope%\"}" ;;
      severity*=*) severity="${line#*= \"}"; severity="${severity%\"}" ;;
    esac
  done < "$config"

  # Emit last check
  [[ -n "$name" && -n "$command" ]] && $handler "$name" "$command" "$triggers" "$scope" "$severity"
}
