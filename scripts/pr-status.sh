#!/bin/bash
# Show PR status and failed pipeline jobs with colors

export GH_PAGER=cat

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

BRANCH=$(git branch --show-current)
printf "${BLUE}[debug] branch: %s${NC}\n" "$BRANCH"

RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')
if [ -z "$RUN_ID" ]; then
  printf "${RED}ERROR: No runs found for branch %s${NC}\n" "$BRANCH"
  exit 1
fi
printf "${BLUE}[debug] run_id: %s${NC}\n" "$RUN_ID"

printf "\n${BLUE}Pipeline Status:${NC}\n"

# Fetch jobs and store in a variable to avoid multiple calls
JOBS_JSON=$(gh run view "$RUN_ID" --json jobs)

# Display each job with its status/conclusion
echo "$JOBS_JSON" | jq -r '.jobs[] | "\(.status)\t\(.conclusion)\t\(.name)"' | while IFS=$'\t' read -r STATUS CONCLUSION NAME; do
    if [ "$STATUS" != "completed" ]; then
        printf "${YELLOW}󱎫 %s${NC}\t%s\n" "$STATUS" "$NAME"
    elif [ "$CONCLUSION" == "success" ]; then
        printf "${GREEN}✔ success${NC}\t%s\n" "$NAME"
    elif [ "$CONCLUSION" == "failure" ]; then
        printf "${RED}✘ failure${NC}\t%s\n" "$NAME"
    else
        printf "${NC}󰄱 %s${NC}\t%s\n" "${CONCLUSION:-pending}" "$NAME"
    fi
done

# Check for failures
FAILED_JOBS=$(echo "$JOBS_JSON" | jq -r '.jobs[] | select(.conclusion == "failure") | "\(.databaseId)\t\(.name)"')

if [ -n "$FAILED_JOBS" ]; then
  printf "\n${RED}Failed jobs detail:${NC}\n"
  while IFS=$'\t' read -r JOB_ID JOB_NAME; do
    printf "\n${RED}=== %s (id: %s) ===${NC}\n" "$JOB_NAME" "$JOB_ID"
    # Show log snippet for failures
    gh run view "$RUN_ID" --log --job "$JOB_ID" 2>&1 | grep -iE "(error:|FAILED|failed)" | head -20
  done <<< "$FAILED_JOBS"
  exit 1
fi

# Check if any jobs are still running
IN_PROGRESS=$(echo "$JOBS_JSON" | jq -r '.jobs[] | select(.status != "completed") | .name')
if [ -n "$IN_PROGRESS" ]; then
  printf "\n${YELLOW}Some jobs are still in progress...${NC}\n"
  exit 0
fi

printf "\n${GREEN}All jobs passed!${NC}\n"
exit 0
