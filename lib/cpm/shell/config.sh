#!/usr/bin/env bash
# config.sh — Parse cpm config file into environment variables.
#
# Config format is a provider — first found wins:
#   cpm.toml → cpm.yml → cpm.json → .cpmrc
#
# Output: exports CPM_* variables for use by check scripts.
# Usage: source lib/cpm/shell/config.sh
#
# @see docs/adr/adr-121-cpm-quality-layer.md

# Find config file (provider pattern: first found wins)
# Search order: root (project manifest), then .config/ (tool config dir)
_cpm_find_config() {
  local root="${CPM_ROOT:-.}"
  for f in \
    "$root/cpm.toml" "$root/cpm.yml" "$root/cpm.json" "$root/.cpmrc" \
    "$root/.config/cpm.toml" "$root/.config/cpm.yml" "$root/.config/cpm.json" "$root/.config/.cpmrc"; do
    [[ -f "$f" ]] && echo "$f" && return
  done
  echo ""
}

# Parse TOML (simple key=value extraction, handles [sections])
_cpm_parse_toml() {
  local file="$1" section=""
  while IFS= read -r line; do
    # Skip comments and empty lines
    [[ "$line" =~ ^[[:space:]]*# ]] && continue
    [[ -z "${line// /}" ]] && continue

    # Section header: [limits] → prefix = LIMITS_
    if [[ "$line" =~ ^\[([a-z._-]+)\]$ ]]; then
      section="${BASH_REMATCH[1]}"
      section="${section//./_}"
      section="${section//-/_}"
      continue
    fi

    # Key = value
    if [[ "$line" =~ ^([a-z_-]+)[[:space:]]*=[[:space:]]*(.+)$ ]]; then
      local key="${BASH_REMATCH[1]}"
      local val="${BASH_REMATCH[2]}"
      key="${key//-/_}"

      # Strip quotes
      val="${val%\"}"
      val="${val#\"}"

      # Build env var name: CPM_<SECTION>_<KEY>
      local var="CPM_${section^^}_${key^^}"
      var="${var//./_}"
      export "$var=$val"
    fi
  done <"$file"
}

# Parse .cpmrc (already env-style)
_cpm_parse_env() {
  set -a
  # shellcheck source=/dev/null
  source "$1"
  set +a
}

# Main: detect format and parse
_cpm_config_file=$(_cpm_find_config)

if [[ -n "$_cpm_config_file" ]]; then
  case "$_cpm_config_file" in
    *.toml) _cpm_parse_toml "$_cpm_config_file" ;;
    *.yml|*.yaml) echo "WARNING: YAML parser not yet implemented" >&2 ;;
    *.json) echo "WARNING: JSON parser not yet implemented" >&2 ;;
    .cpmrc) _cpm_parse_env "$_cpm_config_file" ;;
  esac
fi

# Expose convenience vars with defaults (scripts use these)
export CPM_LIMIT_SOURCE_LINES="${CPM_LIMITS_SOURCE_LINES:-600}"
export CPM_LIMIT_HEADER_LINES="${CPM_LIMITS_HEADER_LINES:-300}"
export CPM_LIMIT_TEST_LINES="${CPM_LIMITS_TEST_LINES:-700}"
export CPM_LIMIT_SCRIPT_LINES="${CPM_LIMITS_SCRIPT_LINES:-200}"
export CPM_LIMIT_FILES_PER_DIR="${CPM_LIMITS_FILES_PER_DIR:-20}"
export CPM_PROJECT_LEVEL="${CPM_PROJECT_LEVEL:-0}"
export CPM_PROJECT_NAME="${CPM_PROJECT_NAME:-unknown}"
