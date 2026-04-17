#!/usr/bin/env bash
#
# pipeline-status.sh — Show latest pipeline status for current branch
# GH_PAGER=cat prevents gh from opening an interactive pager

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi


export GH_PAGER=cat

BRANCH=$(git branch --show-current)
RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')

if [ -z "$RUN_ID" ]; then
  echo "No runs found for branch $BRANCH"
  exit 1
fi

STATUS=$(gh run view "$RUN_ID" --json status,conclusion -q '"status=\(.status) conclusion=\(.conclusion)"')
echo "Branch:  $BRANCH"
echo "Run ID:  $RUN_ID"
echo "Result:  $STATUS"
