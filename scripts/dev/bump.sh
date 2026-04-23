#!/usr/bin/env bash
# Auto-bump version based on conventional commits since last tag.
# Creates a local git tag. Usage: bump.sh [major|minor|patch]
# Without argument, analyzes commits to determine bump type.
set -o errexit
set -o nounset
set -o pipefail

LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
VER="${LAST_TAG#v}"
IFS='.' read -r major minor patch <<< "$VER"

PART="${1:-}"
if [[ -z "$PART" ]]; then
  COMMITS=$(git log "${LAST_TAG}..HEAD" --pretty=format:"%s" 2>/dev/null || git log --pretty=format:"%s")
  PART="patch"
  if echo "$COMMITS" | grep -qiE '^feat(\(.*\))?!:|BREAKING CHANGE'; then
    PART="major"
  elif echo "$COMMITS" | grep -qiE '^feat(\(.*\))?:'; then
    PART="minor"
  fi
fi

case "$PART" in
  major) major=$((major + 1)); minor=0; patch=0 ;;
  minor) minor=$((minor + 1)); patch=0 ;;
  patch) patch=$((patch + 1)) ;;
  *) echo "Usage: bump.sh [major|minor|patch]"; exit 1 ;;
esac

NEW="${major}.${minor}.${patch}"
TAG="v${NEW}"

git tag "$TAG"
echo "${VER} -> ${NEW} (tagged ${TAG})"
echo "Push with: git push origin ${TAG}"
