#!/usr/bin/env bash
#
# Create a GitHub issue using the gh CLI
# Usage: ./scripts/gh/create-issue.sh "Issue Title" "Issue Description"

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
TITLE="${1:-}"
BODY="${2:-}"

if [ -z "$TITLE" ] || [ -z "$BODY" ]; then
  echo "Usage: $0 \"title\" \"description\""
  exit 1
fi

# Check if gh CLI is installed
if ! command -v gh &>/dev/null; then
  echo "ERROR: GitHub CLI (gh) is not installed."
  exit 1
fi

# Check if authenticated
if ! gh auth status &>/dev/null; then
  echo "ERROR: gh CLI is not authenticated. Run 'gh auth login' first."
  exit 1
fi

echo "Creating issue: $TITLE..."

# Create the issue and capture the URL
ISSUE_URL=$(gh issue create --title "$TITLE" --body "$BODY")

echo "SUCCESS: Issue created at $ISSUE_URL"

# Optionally download issues to sync local cache
if [ -f "./scripts/gh/download-issues.sh" ]; then
  echo "Syncing local issue cache..."
  ./scripts/gh/download-issues.sh >/dev/null || true
fi
