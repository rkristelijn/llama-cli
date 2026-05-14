#!/usr/bin/env bash
# format-yaml.sh — Auto-format YAML files (strip trailing whitespace).
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true

if ! command -v yamllint >/dev/null; then
  print_step 1 "format-yaml" skip "yamllint not installed"
  exit 0
fi

print_header "formatting yaml (stripping trailing whitespace)..."
# Find YAML files reported by yamllint and clean them
yamllint -d relaxed -f parsable .github/ 2>/dev/null | awk -F: '{print $1}' | sort -u | while read -r f; do
  if [[ -f "$f" ]]; then
    sed -i.bak 's/[[:space:]]*$//' "$f" && rm -f "$f.bak"
  fi
done
