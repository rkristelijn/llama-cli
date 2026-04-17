#!/usr/bin/env bash
#
# Show PR status and failed pipeline jobs with colors and timings.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

export GH_PAGER=cat

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
DIM='\033[2m'
NC='\033[0m'

DEBUG=false
if [ "${1:-}" = "--debug" ]; then
  DEBUG=true
fi

BRANCH=$(git branch --show-current)
printf "${BLUE}[info] branch: %s${NC}\n" "$BRANCH"

RUN_ID=$(gh run list --branch "$BRANCH" --limit 1 --json databaseId -q '.[0].databaseId')
if [ -z "$RUN_ID" ] || [ "$RUN_ID" = "null" ]; then
  printf "${RED}ERROR: No runs found for branch %s${NC}\n" "$BRANCH"
  exit 1
fi
printf "${BLUE}[info] run_id: %s${NC}\n" "$RUN_ID"

printf "\n${BLUE}Pipeline Status:${NC}\n"

JOBS_JSON=$(gh run view "$RUN_ID" --json jobs)

if [ "$DEBUG" = "true" ]; then
  echo "$JOBS_JSON" | jq -c '.jobs[] | {status, conclusion, name}'
fi

# Display each job with status, conclusion, and timing
echo "$JOBS_JSON" | jq -r '.jobs[] | "\(.status)|\(.conclusion // "-")|\(.name)|\(.startedAt // "")|\(.completedAt // "")"' | while IFS='|' read -r STATUS CONCLUSION NAME STARTED COMPLETED; do
    # Calculate duration
    DURATION=""
    if [ -n "$STARTED" ] && [ "$STARTED" != "null" ]; then
      START_S=$(date -d "$STARTED" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%S" "${STARTED%%.*}" +%s 2>/dev/null || echo "")
      if [ -n "$START_S" ]; then
        if [ -n "$COMPLETED" ] && [ "$COMPLETED" != "null" ] && [ "$COMPLETED" != "0001-01-01T00:00:00Z" ]; then
          END_S=$(date -d "$COMPLETED" +%s 2>/dev/null || date -j -f "%Y-%m-%dT%H:%M:%S" "${COMPLETED%%.*}" +%s 2>/dev/null || echo "")
          [ -n "$END_S" ] && DURATION=" ${DIM}($(( END_S - START_S ))s)${NC}"
        else
          NOW_S=$(date +%s)
          DURATION=" ${YELLOW}($(( NOW_S - START_S ))s running)${NC}"
        fi
      fi
    fi

    if [ "$STATUS" != "completed" ]; then
        printf "${YELLOW}󱎫 %-12s${NC} %s${DURATION}\n" "$STATUS" "$NAME"
    elif [ "$CONCLUSION" = "success" ]; then
        printf "${GREEN}✔ %-12s${NC} %s${DURATION}\n" "success" "$NAME"
    elif [ "$CONCLUSION" = "failure" ]; then
        printf "${RED}✘ %-12s${NC} %s${DURATION}\n" "failure" "$NAME"
    else
        printf "${NC}󰄱 %-12s${NC} %s${DURATION}\n" "$CONCLUSION" "$NAME"
    fi
done

# Check for failures
FAILED_JOBS=$(echo "$JOBS_JSON" | jq -r '.jobs[] | select(.conclusion == "failure") | "\(.databaseId)\t\(.name)"')

if [ -n "$FAILED_JOBS" ]; then
  printf "\n${RED}Failed jobs detail:${NC}\n"

  LOG_FILE="$(mktemp)"
  trap 'rm -f "$LOG_FILE"' EXIT
  gh run view "$RUN_ID" --log > "$LOG_FILE" 2>/dev/null

  while IFS=$'\t' read -r JOB_ID JOB_NAME; do
    printf "\n${RED}=== %s (id: %s) ===${NC}\n" "$JOB_NAME" "$JOB_ID"
    # Anchor grep to job name at start of line (log format: "JOBNAME\tSTEP\tLINE")
    snippet=$(grep -P "^${JOB_NAME}\t" "$LOG_FILE" | grep -i "error\|fail\|missing\|warning\|\#\#\[error\]" | tail -n 30 || true)
    if [ -n "$snippet" ]; then
      # Strip job name prefix and timestamps for readability
      printf '%s\n' "$snippet" | sed "s/^${JOB_NAME}\t//" | sed 's/[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}T[0-9:.]*Z //'
    else
      echo "  No error lines found. Run with --debug or check GitHub Actions UI."
    fi
  done <<< "$FAILED_JOBS"
  rm -f "$LOG_FILE"
  trap - EXIT
  exit 1
fi

IN_PROGRESS=$(echo "$JOBS_JSON" | jq -r '.jobs[] | select(.status != "completed") | .name')
if [ -n "$IN_PROGRESS" ]; then
  printf "\n${YELLOW}Some jobs are still in progress...${NC}\n"
  exit 0
fi

printf "\n${GREEN}All jobs passed!${NC}\n"
exit 0
