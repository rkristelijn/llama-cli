#!/usr/bin/env bash
#
# Download GitHub issues and persist in ./.cache/issues/

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi


REPO="${1:-$(git remote get-url origin 2>/dev/null | sed 's/.*github.com[/:]//' | sed 's/\.git$//')}"
OWNER="${2:-${REPO%/*}}"
NAME="${3:-${REPO#*/}}"
OUTPUT_DIR="./.cache/issues"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Downloading issues from $OWNER/$NAME..."

# Use gh CLI to fetch issues (handles auth automatically)
gh issue list --repo "$OWNER/$NAME" --state all --limit 100 --json number,title,body | \
  jq -c '.[]' | \
while read -r ISSUE; do
  NR=$(echo "$ISSUE" | jq -r '.number')
  TITLE=$(echo "$ISSUE" | jq -r '.title')
  BODY=$(echo "$ISSUE" | jq -r '.body // ""')

  FILENAME="$OUTPUT_DIR/${NR}.md"

  # Create markdown with Doxyfile header
  cat > "$FILENAME" << HEREDOC
/**
 * @file ${NR}.md
 * @brief Issue #$NR
 * @issue $NR
 * @url https://github.com/$OWNER/$NAME/issues/$NR
 */

/*
# Issue #$NR

$BODY
 */

HEREDOC

  echo "Saved: $FILENAME"
done

echo "Done. Issues saved to $OUTPUT_DIR/"
