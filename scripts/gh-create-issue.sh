#!/bin/bash
# Create a GitHub issue using the gh CLI
# Usage: ./scripts/gh-create-issue.sh "Issue Title" "Issue Description"

TITLE="$1"
BODY="$2"

if [ -z "$TITLE" ] || [ -z "$BODY" ]; then
  echo "Usage: $0 \"title\" \"description\""
  exit 1
fi

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
  echo "ERROR: GitHub CLI (gh) is not installed."
  exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
  echo "ERROR: gh CLI is not authenticated. Run 'gh auth login' first."
  exit 1
fi

echo "Creating issue: $TITLE..."

# Create the issue and capture the URL
ISSUE_URL=$(gh issue create --title "$TITLE" --body "$BODY")

if [ $? -eq 0 ]; then
  echo "SUCCESS: Issue created at $ISSUE_URL"
  
  # Optionally download issues to sync local cache
  if [ -f "./scripts/download-issues.sh" ]; then
    echo "Syncing local issue cache..."
    ./scripts/download-issues.sh > /dev/null
  fi
else
  echo "ERROR: Failed to create issue."
  exit 1
fi
