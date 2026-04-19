#!/usr/bin/env bash
#
# Sync backlog ↔ GitHub issues
# 1. Download new issues into .cache/issues/
# 2. Create GitHub issues for backlog items missing an issue link
#
# Usage: ./scripts/gh/sync-backlog.sh [--dry-run]

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

DRY_RUN=false
[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=true

BACKLOG_DIR="docs/backlog"
REPO="$(git remote get-url origin 2>/dev/null | sed 's/.*github.com[/:]//' | sed 's/\.git$//')"

# Step 1: Download issues
echo "=== Step 1: Downloading issues ==="
if [[ "$DRY_RUN" == false ]]; then
  ./scripts/gh/download-issues.sh
else
  echo "(dry-run) Would run: ./scripts/gh/download-issues.sh"
fi

# Step 2: Create issues for backlog items without issue links
echo ""
echo "=== Step 2: Checking backlog for missing issues ==="

for file in "$BACKLOG_DIR"/[0-9]*.md; do
  [[ -f "$file" ]] || continue
  basename="$(basename "$file")"

  # Skip README
  [[ "$basename" == "README.md" ]] && continue

  # Check if file has a GitHub issue link
  if grep -q 'github.com/.*/issues/[0-9]' "$file"; then
    echo "✓ $basename — has issue link"
    continue
  fi

  # Extract title from first heading
  title="$(head -1 "$file" | sed 's/^# //')"

  if [[ "$DRY_RUN" == true ]]; then
    echo "⊕ $basename — would create issue: $title"
  else
    echo "⊕ $basename — creating issue: $title"
    issue_url="$(gh issue create --repo "$REPO" --title "$title" --body "Backlog item: $basename

See [docs/backlog/$basename](https://github.com/$REPO/blob/main/docs/backlog/$basename) for details.")"

    # Extract issue number from URL
    issue_nr="$(echo "$issue_url" | grep -o '[0-9]*$')"

    # Add issue link to the backlog file (after the Status line)
    sed -i.bak "s|^\*Status\*: \(.*\) · \*Date\*: \(.*\)|\*Status\*: \1 · *Date*: \2 · *Issue*: [#${issue_nr}](${issue_url})|" "$file"
    rm -f "${file}.bak"

    echo "  → Created $issue_url, updated $basename"
  fi
done

echo ""
echo "Done."
