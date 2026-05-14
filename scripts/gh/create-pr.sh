#!/usr/bin/env bash
#
# Create a pull request for the current branch
# Usage: ./scripts/gh/create-pr.sh [title] [base]

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
set -euo pipefail

BRANCH=$(git rev-parse --abbrev-ref HEAD)
TITLE="${1:-$(git log -1 --pretty=%B | head -1)}"
BASE="${2:-main}"

if [ "$BRANCH" = "$BASE" ]; then
  echo "ERROR: Cannot create PR from $BASE to $BASE"
  exit 1
fi

echo "Creating PR: $BRANCH → $BASE"
echo "Title: $TITLE"

# Get commit messages for body
BODY=$(git log --pretty=format:"- %s" "$BASE..$BRANCH" | head -10)

gh pr create \
  --draft \
  --title "$TITLE" \
  --body "## Changes

$BODY

## Testing

- \`make check\` passes locally
- Heavy checks (coverage, sanitizers) run when draft is removed: \`gh pr ready\`" \
  --base "$BASE" \
  --head "$BRANCH"

echo "[PASS] Draft PR created — run 'make pr-ready' when done"
