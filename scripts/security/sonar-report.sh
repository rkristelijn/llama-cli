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
  echo "  ERROR: SONAR_TOKEN not set. See: https://sonarcloud.io/account/security" >&2
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
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&facets=severities,rules,types&ps=1" |
    python3 -c "
import json, sys
data = json.load(sys.stdin)
total = data['total']
facets = {f['property']: f['values'] for f in data.get('facets', [])}

print(f'==> SonarCloud Report: ${PROJECT}')
print(f'    Total open issues: {total}')
print()

print('    Severity')
for v in facets.get('severities', []):
    bar_len = (v['count'] * 30 // total) if total else 0
    bar = '#' * max(1, bar_len) if total else ''
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
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&severities=${severity}&ps=50" |
    python3 -c "
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
# Print actionable issues (BLOCKER + CRITICAL + BUG + VULNERABILITY).
#######################################
print_actionable() {
  curl -s ${AUTH} \
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&severities=BLOCKER&ps=50" \
    "${API}/issues/search?projectKeys=${PROJECT}&statuses=OPEN&types=BUG,VULNERABILITY&ps=50" |
    python3 -c "
import json, sys, subprocess, os

auth = '${AUTH}'.split()
api = '${API}'
project = '${PROJECT}'

def fetch(params):
    import urllib.request
    url = f'{api}/issues/search?projectKeys={project}&statuses=OPEN&{params}&ps=50'
    req = urllib.request.Request(url)
    token = os.environ.get('SONAR_TOKEN', '')
    import base64
    creds = base64.b64encode(f'{token}:'.encode()).decode()
    req.add_header('Authorization', f'Basic {creds}')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read())

# Fetch BLOCKERs and BUGs/VULNs separately, deduplicate
blockers = fetch('severities=BLOCKER')
bugs = fetch('types=BUG,VULNERABILITY')

seen = set()
actionable = []
for issue in blockers.get('issues', []) + bugs.get('issues', []):
    key = issue.get('key')
    if key not in seen:
        seen.add(key)
        actionable.append(issue)

if not actionable:
    print('    No actionable issues (BLOCKERs, BUGs, VULNERABILITYs).')
    sys.exit(0)

print()
print(f'==> {len(actionable)} actionable issues (BLOCKER/BUG/VULNERABILITY):')
print()
for i in actionable:
    comp = i.get('component', '').replace(f'{project}:', '')
    line = i.get('line', '?')
    rule = i.get('rule', '?')
    sev = i.get('severity', '?')
    typ = i.get('type', '?')
    msg = i.get('message', '')
    print(f'  [{sev}] {comp}:{line}')
    print(f'    [{rule}] ({typ}) {msg}')
    print()
"
}

#######################################
# Main
#######################################
main() {
  print_summary

  if [[ -n "${DETAIL_SEVERITY}" ]]; then
    # Explicit severity filter (backward compatible)
    print_detail "${DETAIL_SEVERITY}"
  else
    # Default: show actionable issues that need fixing
    print_actionable
  fi
}

main "$@"
