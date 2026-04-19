#!/usr/bin/env bash
# format-md.sh — Auto-format Markdown files using rumdl.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v rumdl >/dev/null; then
  echo "  [skip] rumdl not installed"
  exit 0
fi

echo "==> formatting markdown..."
rumdl fmt .
echo "  [done] format-md"
