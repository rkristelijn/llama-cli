#!/bin/bash
# pipeline-status.sh — Show latest pipeline status for current branch
# GH_PAGER=cat prevents gh from opening an interactive pager

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
