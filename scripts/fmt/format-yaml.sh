#!/usr/bin/env bash
# format-yaml.sh — Auto-format YAML files (strip trailing whitespace).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v yamllint >/dev/null; then
  echo "  [skip] yamllint not installed"
  exit 0
fi

echo "==> formatting yaml (stripping trailing whitespace)..."
# Find YAML files reported by yamllint and clean them
yamllint -d relaxed -f parsable .github/ 2>/dev/null | awk -F: '{print $1}' | sort -u | while read -r f; do
  if [[ -f "$f" ]]; then
    sed -i.bak 's/[[:space:]]*$//' "$f" && rm -f "$f.bak"
  fi
done
echo "  [done] format-yaml"
