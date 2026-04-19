#!/usr/bin/env bash
# format-scripts.sh — Auto-format shell scripts using shfmt.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v shfmt >/dev/null; then
  echo "  [skip] shfmt not installed"
  exit 0
fi

echo "==> formatting scripts..."
find scripts e2e -name '*.sh' -exec shfmt -i 2 -w {} \;
echo "  [done] format-scripts"
