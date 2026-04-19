#!/usr/bin/env bash
# lint-yaml.sh — Run yamllint on YAML files.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

if ! command -v yamllint >/dev/null; then
  echo "  [skip] yamllint not installed"
  exit 0
fi

echo "==> linting yaml..."
yamllint -c .config/yamllint.yml .github/
echo "  [done] lint-yaml"
