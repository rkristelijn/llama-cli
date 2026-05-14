#!/usr/bin/env bash
# lint-yaml.sh — Run yamllint on YAML files.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
if ! command -v yamllint >/dev/null; then
  print_step "" "$(basename "$0" .sh)" skip "yamllint not installed"
  exit 0
fi

print_header "linting yaml..."
yamllint -c .config/yamllint.yml .github/
