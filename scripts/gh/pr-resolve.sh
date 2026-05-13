#!/usr/bin/env bash
#
# pr-resolve.sh — Interactive loop to review, fix, and resolve PR feedback.
# lint-exempt: max-exits (multiple exit states: error, skip, success)
#
# Walks through unresolved CodeRabbit threads one by one.
# For each: shows the finding, lets you fix it, then resolves with a message.
#
# Requires: gh CLI (authenticated), jq
#
# Usage:
#   bash scripts/gh/pr-resolve.sh           # interactive loop on current PR
#   bash scripts/gh/pr-resolve.sh --list    # just list unresolved threads

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
cd "$(dirname "$0")/../.."

LIST_ONLY=0
[[ "${1:-}" == "--list" ]] && LIST_ONLY=1

# Auto-detect PR
BRANCH="$(git branch --show-current)"
PR_NUMBER="$(gh pr view "$BRANCH" --json number --jq .number 2>/dev/null || echo "")"

if [[ -z "$PR_NUMBER" ]]; then
  echo "ERROR: No PR found for branch '$BRANCH'" >&2
  exit 1
fi

echo "==> PR #$PR_NUMBER — Unresolved review threads"
echo ""

# Get PR node ID
PR_ID="$(gh api graphql -f query='
  query($owner: String!, $name: String!, $number: Int!) {
    repository(owner: $owner, name: $name) {
      pullRequest(number: $number) { id }
    }
  }
' -f owner="rkristelijn" -f name="llama-cli" -F number="$PR_NUMBER" \
  --jq '.data.repository.pullRequest.id')"

# Fetch unresolved threads (all reviewers, not just CodeRabbit)
THREADS_JSON="$(gh api graphql -f query='
  query($id: ID!) {
    node(id: $id) {
      ... on PullRequest {
        reviewThreads(first: 100) {
          nodes {
            id
            isResolved
            path
            line
            comments(first: 5) {
              nodes { author { login } body }
            }
          }
        }
      }
    }
  }
' -F id="$PR_ID" --jq '
  [.data.node.reviewThreads.nodes[]
   | select(.isResolved == false)
   | {
       id: .id,
       path: .path,
       line: .line,
       author: .comments.nodes[0].author.login,
       body: .comments.nodes[0].body
     }]
')"

COUNT="$(echo "$THREADS_JSON" | jq 'length')"

if [[ "$COUNT" -eq 0 ]]; then
  echo "  ✓ No unresolved threads. All clean!"
  exit 0
fi

echo "  $COUNT unresolved thread(s)"
echo ""

# List mode — just show them
if [[ "$LIST_ONLY" -eq 1 ]]; then
  echo "$THREADS_JSON" | jq -r '.[] | "  [\(.author)] \(.path // "general"):\(.line // "-")\n    \(.body | split("\n")[0] | .[0:100])\n"'
  exit 0
fi

# Interactive loop
IDX=0
echo "$THREADS_JSON" | jq -c '.[]' | while IFS= read -r thread; do
  IDX=$((IDX + 1))
  PATH_NAME="$(echo "$thread" | jq -r '.path // "general"')"
  LINE="$(echo "$thread" | jq -r '.line // "-"')"
  AUTHOR="$(echo "$thread" | jq -r '.author')"
  THREAD_ID="$(echo "$thread" | jq -r '.id')"
  # Show first 20 lines of the comment body
  BODY="$(echo "$thread" | jq -r '.body' | head -20)"

  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  echo "[$IDX/$COUNT] $PATH_NAME:$LINE  (by $AUTHOR)"
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  echo "$BODY"
  echo ""
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  echo "  [f] Fixed — resolve with message"
  echo "  [s] Skip — move to next"
  echo "  [n] Not applicable — resolve as 'won't fix'"
  echo "  [q] Quit"
  echo ""
  read -rp "  Action: " action

  case "$action" in
  f | F)
    read -rp "  Fix message (enter for default): " msg
    if [[ -z "$msg" ]]; then
      msg="Fixed in $(git rev-parse --short HEAD)."
    fi
    # Reply and resolve
    gh api graphql -f query='
        mutation($threadId: ID!, $body: String!) {
          addPullRequestReviewThreadReply(input: {pullRequestReviewThreadId: $threadId, body: $body}) {
            comment { id }
          }
        }
      ' -f threadId="$THREAD_ID" -f body="$msg" --silent
    gh api graphql -f query='
        mutation($threadId: ID!) {
          resolveReviewThread(input: {threadId: $threadId}) {
            thread { isResolved }
          }
        }
      ' -f threadId="$THREAD_ID" --silent
    echo "  ✓ Resolved: $msg"
    ;;
  n | N)
    gh api graphql -f query='
        mutation($threadId: ID!, $body: String!) {
          addPullRequestReviewThreadReply(input: {pullRequestReviewThreadId: $threadId, body: $body}) {
            comment { id }
          }
        }
      ' -f threadId="$THREAD_ID" -f body="Won't fix — not applicable to this project." --silent
    gh api graphql -f query='
        mutation($threadId: ID!) {
          resolveReviewThread(input: {threadId: $threadId}) {
            thread { isResolved }
          }
        }
      ' -f threadId="$THREAD_ID" --silent
    echo "  ✓ Resolved as won't fix"
    ;;
  s | S)
    echo "  → Skipped"
    ;;
  q | Q)
    echo "  Quit."
    exit 0
    ;;
  *)
    echo "  → Unknown action, skipping"
    ;;
  esac
  echo ""
done

echo "Done."
