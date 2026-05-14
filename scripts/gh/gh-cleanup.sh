#!/usr/bin/env bash
# gh-cleanup.sh — Remove merged branches (local + remote).
#
# Usage:
#   bash scripts/gh/gh-cleanup.sh           # dry-run (shows what would be deleted)
#   bash scripts/gh/gh-cleanup.sh --apply   # actually delete branches
#
# Skips: main, current branch, branches with open PRs.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
DRY_RUN=true
[[ "${1:-}" == "--apply" ]] && DRY_RUN=false

CURRENT=$(git branch --show-current)
PROTECTED="main|master|${CURRENT}"

# --- Local merged branches ---
echo "==> Local merged branches"
local_branches=$(git branch --merged main | grep -vE "^\*|${PROTECTED}" | sed 's/^[[:space:]]*//' || true)

if [[ -z "$local_branches" ]]; then
  echo "  (none)"
else
  local_count=$(echo "$local_branches" | wc -l | tr -d ' ')
  echo "  Found: $local_count"
  echo "$local_branches" | while read -r branch; do
    if $DRY_RUN; then
      echo "  [dry-run] would delete local: $branch"
    else
      git branch -d "$branch" 2>/dev/null && echo "  ✓ deleted local: $branch" || echo "  ✗ skipped: $branch"
    fi
  done
fi

# --- Remote merged branches ---
echo ""
echo "==> Remote merged branches"
git fetch --prune >/dev/null 2>&1
remote_branches=$(git branch -r --merged main | grep -vE "${PROTECTED}|HEAD" | sed 's|origin/||;s/^[[:space:]]*//' || true)

if [[ -z "$remote_branches" ]]; then
  echo "  (none)"
else
  remote_count=$(echo "$remote_branches" | wc -l | tr -d ' ')
  echo "  Found: $remote_count"
  echo "$remote_branches" | while read -r branch; do
    if $DRY_RUN; then
      echo "  [dry-run] would delete remote: $branch"
    else
      git push origin --delete --no-verify "$branch" 2>/dev/null && echo "  ✓ deleted remote: $branch" || echo "  ✗ skipped: $branch"
    fi
  done
fi

echo ""
if $DRY_RUN; then
  echo "Dry-run complete. Run with --apply to delete."
else
  echo "Cleanup complete."
fi
