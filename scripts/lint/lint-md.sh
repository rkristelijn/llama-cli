#!/usr/bin/env bash
# lint-md.sh — Run rumdl checks on Markdown files.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v rumdl >/dev/null; then
  echo "  [skip] rumdl not installed"
  exit 0
fi

echo "==> linting markdown..."
rumdl check .
echo "  [done] lint-md"
