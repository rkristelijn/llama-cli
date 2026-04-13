#!/bin/bash
# Show PR status and failed pipeline jobs with colors

export GH_PAGER=cat

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

DEBUG=false
if [ "$1" == "--debug" ]; then
  DEBUG=true
fi

BRANCH=$(git branch --show-current)
printf "${BLUE}[info] branch: %s${NC}\n" "$BRANCH"

RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')
if [ -z "$RUN_ID" ]; then
  printf "${RED}ERROR: No runs found for branch %s${NC}\n" "$BRANCH"
  exit 1
fi
printf "${BLUE}[info] run_id: %s${NC}\n" "$RUN_ID"

printf "\n${BLUE}Pipeline Status:${NC}\n"

# Fetch jobs and store in a variable to avoid multiple calls
JOBS_JSON=$(gh run view "$RUN_ID" --json jobs)

if [ "$DEBUG" = true ]; then
  printf "${YELLOW}[debug] raw jobs data:${NC}\n"
  echo "$JOBS_JSON" | jq -c '.jobs[] | {status, conclusion, name}'
  printf "\n"
fi

# Display each job with its status/conclusion
# We use // "-" to ensure the second field (conclusion) is never empty/null.
# We also use a more robust separator if names have spaces.
echo "$JOBS_JSON" | jq -r '.jobs[] | "\(.status)|\(.conclusion // "-")|\(.name)"' | while IFS='|' read -r STATUS CONCLUSION NAME; do
    if [ "$STATUS" != "completed" ]; then
        printf "${YELLOW}󱎫 %-12s${NC} %s\n" "$STATUS" "$NAME"
    elif [ "$CONCLUSION" == "success" ]; then
        printf "${GREEN}✔ %-12s${NC} %s\n" "success" "$NAME"
    elif [ "$CONCLUSION" == "failure" ]; then
        printf "${RED}✘ %-12s${NC} %s\n" "failure" "$NAME"
    else
        # Handle skipped, cancelled, or other statuses
        printf "${NC}󰄱 %-12s${NC} %s\n" "$CONCLUSION" "$NAME"
    fi
done

# Check for failures
FAILED_JOBS=$(echo "$JOBS_JSON" | jq -r '.jobs[] | select(.conclusion == "failure") | "\(.databaseId)\t\(.name)"')

if [ -n "$FAILED_JOBS" ]; then
  printf "\n${RED}Failed jobs detail:${NC}\n"
  while IFS=$'\t' read -r JOB_ID JOB_NAME; do
    printf "\n${RED}=== %s (id: %s) ===${NC}\n" "$JOB_NAME" "$JOB_ID"
    # Show log for the failed job, focusing on the actual step output
    gh run view "$RUN_ID" --log --job "$JOB_ID" 2>&1 | tail -50
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
