#!/usr/bin/env bash
#
# Show PR status and failed pipeline jobs with colors
# 
# Activity Diagram:
# [Start] -> [Get Branch] -> [Query Run ID] -> [Fetch Jobs JSON] 
#                                                   |
#                                           [Display Status]
#                                                   |
#                                       {Failures?} --(Yes)--> [Fetch Full Log] -> [Extract Failed Logs] -> [Exit 1]
#                                       |   |
#                                       |--(No)--> [Check In-Progress] --(Yes)--> [Exit 0]
#                                       |                                   |
#                                       |                                 --(No)--> [Exit 0 (Success)]

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi


export GH_PAGER=cat

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

DEBUG=false
if [ "${1:-}" = "--debug" ]; then
  DEBUG=true
fi

# Detect current branch to align CI runs with local development
BRANCH=$(git branch --show-current)
printf "${BLUE}[info] branch: %s${NC}\n" "$BRANCH"

# Line 21: Query the GitHub API for the single most recent workflow run associated with the current branch
# We use the GitHub CLI (gh) json output and a jq query (-q) to extract the primary database ID.
RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')
if [ -z "$RUN_ID" ]; then
  printf "${RED}ERROR: No runs found for branch %s${NC}\n" "$BRANCH"
  exit 1
fi
printf "${BLUE}[info] run_id: %s${NC}\n" "$RUN_ID"

printf "\n${BLUE}Pipeline Status:${NC}\n"

# Fetch jobs and store in a variable to avoid multiple calls
CMD="gh run view $RUN_ID --json jobs"
if [ "$DEBUG" = "true" ]; then
  printf "${YELLOW}[debug] executing: %s${NC}\n" "$CMD"
fi
JOBS_JSON=$($CMD)

if [ "$DEBUG" = "true" ]; then
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
    elif [ "$CONCLUSION" = "success" ]; then
        printf "${GREEN}✔ %-12s${NC} %s\n" "success" "$NAME"
    elif [ "$CONCLUSION" = "failure" ]; then
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
  
  # Fetch full logs for the run as a fallback (gh CLI limitation: individual job logs often fail)
  LOG_FILE="$(mktemp)"
  trap 'rm -f "$LOG_FILE"' EXIT
  gh run view "$RUN_ID" --log > "$LOG_FILE" 2>/dev/null
  
  while IFS=$'\t' read -r JOB_ID JOB_NAME; do
    printf "\n${RED}=== %s (id: %s) ===${NC}\n" "$JOB_NAME" "$JOB_ID"
    snippet=$(grep -A 50 "$JOB_NAME" "$LOG_FILE" | grep -v "^[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}T" | tail -n 50 || true)
    if [ -n "$snippet" ]; then
      printf '%s\n' "$snippet"
    else
      echo "Logs not found for $JOB_NAME"
    fi
  done <<< "$FAILED_JOBS"
  rm -f "$LOG_FILE"
  trap - EXIT
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
