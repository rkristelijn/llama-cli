#!/usr/bin/env bash
#
# Create a pull request for the current branch
# Usage: ./scripts/gh/create-pr.sh [title] [base]

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
