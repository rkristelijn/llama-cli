#!/usr/bin/env bash
# lint-md.sh — Run rumdl checks on Markdown files.
# Config: .config/rumdl.toml

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
if ! command -v rumdl >/dev/null; then
  print_step "" "$(basename "$0" .sh)" skip "rumdl not installed"
  exit 0
fi

print_header "linting markdown..."
rumdl check .
