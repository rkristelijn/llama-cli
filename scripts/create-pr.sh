#!/bin/bash
# Create a pull request for the current branch
# Usage: ./scripts/create-pr.sh [title] [base]

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
  --title "$TITLE" \
  --body "## Changes

$BODY

## Testing

Run \`make check\` to verify all quality gates pass." \
  --base "$BASE" \
  --head "$BRANCH"

echo "✅ PR created"
