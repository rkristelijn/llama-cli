#!/usr/bin/env bash
#
# sonar-report.sh — Print SonarCloud issue summary for llama-cli.
#
# Queries the SonarCloud API and prints a breakdown by severity, type,
# and top rules. Optionally shows individual issues for a specific severity.
#
# Requires: SONAR_TOKEN environment variable
#           curl, python3
#
# Usage:
#   make sonar-report              # summary only
#   make sonar-report ARGS=BLOCKER # show BLOCKER issues in detail

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

cd "$(dirname "$0")/../.."

# shellcheck disable=SC1091
[[ -f .env ]] && source .env

if [[ -z "${SONAR_TOKEN:-}" ]]; then
  echo "  ERROR: SONAR_TOKEN not set. See: https://sonarcloud.io/account/security"
  exit 1
fi

PROJECT="rkristelijn_llama-cli"
API="https://sonarcloud.io/api"
AUTH="-u ${SONAR_TOKEN}:"
DETAIL_SEVERITY="${1:-}"

#######################################
# Print summary: totals by severity, type, and top rules.
#######################################
print_summary() {
  curl -s ${AUTH} \
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&facets=severities,rules,types&ps=1" \
  | python3 -c "
import json, sys
data = json.load(sys.stdin)
total = data['total']
facets = {f['property']: f['values'] for f in data.get('facets', [])}

print(f'==> SonarCloud Report: ${PROJECT}')
print(f'    Total open issues: {total}')
print()

print('    Severity')
for v in facets.get('severities', []):
    bar = '#' * max(1, v['count'] * 30 // total)
    print(f'      {v[\"val\"]:10s} {v[\"count\"]:5d}  {bar}')
print()

print('    Type')
for v in facets.get('types', []):
    if v['count'] > 0:
        print(f'      {v[\"val\"]:15s} {v[\"count\"]:5d}')
print()

print('    Top 10 Rules')
for v in facets.get('rules', [])[:10]:
    print(f'      {v[\"val\"]:20s} {v[\"count\"]:5d}')
print()
print(f'    Dashboard: https://sonarcloud.io/project/overview?id=${PROJECT}')
"
}

#######################################
# Print detailed issues for a given severity.
#######################################
print_detail() {
  local severity="$1"
  curl -s ${AUTH} \
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&severities=${severity}&ps=50" \
  | python3 -c "
import json, sys
data = json.load(sys.stdin)
issues = data.get('issues', [])
print()
print(f'==> {len(issues)} ${severity} issues:')
print()
for i in issues:
    comp = i.get('component', '').replace('${PROJECT}:', '')
    line = i.get('line', '?')
    rule = i.get('rule', '?')
    msg = i.get('message', '')
    print(f'  {comp}:{line}')
    print(f'    [{rule}] {msg}')
    print()
"
}

#######################################
# Main
#######################################
main() {
  print_summary

  if [[ -n "${DETAIL_SEVERITY}" ]]; then
    print_detail "${DETAIL_SEVERITY}"
  fi
}

main "\$@"
