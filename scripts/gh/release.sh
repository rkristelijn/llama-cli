#!/usr/bin/env bash
# release.sh — Trigger a GitHub Actions release from the CLI.
#
# Usage:
#   bash scripts/gh/release.sh          # trigger release on current branch
#   bash scripts/gh/release.sh --dry-run # show what would happen
#
# Requires: gh CLI authenticated with repo access.
# The release workflow auto-calculates the version from conventional commits.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Ensure we're on main before releasing
# Safety: prevents accidental release from feature branches
BRANCH=$(git branch --show-current)
DRY_RUN=0

for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=1 ;;
  esac
done

if [[ "$BRANCH" != "main" ]]; then
  echo "  ✗ Must be on main branch to release (currently on: $BRANCH)"
  exit 1
fi

# Show what will be released
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
COMMITS=$(git log "${LAST_TAG}..HEAD" --oneline 2>/dev/null | head -20)
COUNT=$(git log "${LAST_TAG}..HEAD" --oneline 2>/dev/null | wc -l | tr -d ' ')

echo "  Release from: $BRANCH"
echo "  Last tag:     $LAST_TAG"
echo "  Commits:      $COUNT since last tag"
echo ""

if [[ "$COUNT" -eq 0 ]]; then
  echo "  ✗ No commits since $LAST_TAG — nothing to release"
  exit 1
fi

echo "  Recent commits:"
echo "$COMMITS" | sed 's/^/    /'
echo ""

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "  [dry-run] Would trigger: gh workflow run release.yml --ref main"
  exit 0
fi

# Trigger the release workflow
gh workflow run release.yml --ref main
echo "  ✓ Release workflow triggered"
echo "  → Watch progress: gh run list --workflow=release.yml --limit=1"
