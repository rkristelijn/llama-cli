#!/bin/bash
# Show PR status and failed pipeline jobs

export GH_PAGER=cat

BRANCH=$(git branch --show-current)
echo "[debug] branch: $BRANCH"

RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')
echo "[debug] run_id: $RUN_ID"

if [ -z "$RUN_ID" ]; then
  echo "ERROR: No runs found for branch $BRANCH"
  exit 1
fi

# Show all jobs with their conclusion for visibility
echo ""
echo "[debug] all jobs + conclusions:"
gh run view "$RUN_ID" --json jobs -q '.jobs[] | "\(.conclusion)\t\(.name)"'
echo ""

FAILED_JOBS=$(gh run view "$RUN_ID" --json jobs -q '.jobs[] | select(.conclusion == "failure") | "\(.databaseId)\t\(.name)"')

if [ -z "$FAILED_JOBS" ]; then
  echo "All jobs passed!"
  exit 0
fi

echo "Failed jobs:"
while IFS=$'\t' read -r JOB_ID JOB_NAME; do
  echo ""
  echo "=== $JOB_NAME (id: $JOB_ID) ==="
  gh run view "$RUN_ID" --log --job "$JOB_ID" 2>&1 | grep -E "(error:|Error:|FAILED|failed)" | head -20
done <<< "$FAILED_JOBS"
