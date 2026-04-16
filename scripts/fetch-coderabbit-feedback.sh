#!/bin/bash
# Download CodeRabbit review comments from a PR and persist in .cache/pr/

REPO="${1:-$(git remote get-url origin 2>/dev/null | sed 's/.*github.com[/:]//' | sed 's/\.git$//')}"
BRANCH="$(git branch --show-current 2>/dev/null)"
PR="${2:-$(gh pr view "$BRANCH" --json number --jq .number 2>/dev/null || echo "")}"

if [ -z "$PR" ]; then
  echo "Error: could not detect PR number. Pass it as argument: $0 [REPO] PR_NUMBER" >&2
  exit 1
fi
OUTPUT_DIR=".cache/pr"

mkdir -p "$OUTPUT_DIR"

echo "Downloading CodeRabbit feedback for PR #$PR from $REPO..."

# Save review summary
gh api "repos/$REPO/pulls/$PR/reviews" \
  --jq '.[] | select(.user.login == "coderabbitai[bot]") | .body' \
  > "$OUTPUT_DIR/${PR}-summary.md"

echo "Saved: $OUTPUT_DIR/${PR}-summary.md"

# Save inline comments as individual files per path
gh api "repos/$REPO/pulls/$PR/comments" \
  --jq '.[] | select(.user.login == "coderabbitai[bot]") | {path, line, body}' | \
  jq -s 'group_by(.path) | .[]' | \
  jq -c '.[0].path as $p | {path: $p, comments: .}' | \
while read -r GROUP; do
  PATH_NAME=$(echo "$GROUP" | jq -r '.path')
  SAFE_NAME=$(echo "$PATH_NAME" | tr '/' '_')
  FILENAME="$OUTPUT_DIR/${PR}-${SAFE_NAME}.md"

  echo "# $PATH_NAME" > "$FILENAME"
  echo "" >> "$FILENAME"

  echo "$GROUP" | jq -r '.comments[] | "## Line \(.line // "N/A")\n\n\(.body)\n\n---\n"' >> "$FILENAME"

  echo "Saved: $FILENAME"
done

echo "Done. Feedback saved to $OUTPUT_DIR/"
