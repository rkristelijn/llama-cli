#!/usr/bin/env bash
# Bump the project version in VERSION file.
# Usage: bump.sh <major|minor|patch>
set -o errexit
set -o nounset
set -o pipefail

PART="${1:-}"
VERSION_FILE="VERSION"

if [[ ! "$PART" =~ ^(major|minor|patch)$ ]]; then
  echo "Usage: make bump PART=<major|minor|patch>"
  exit 1
fi

OLD=$(tr -d '[:space:]' < "$VERSION_FILE")
IFS='.' read -r major minor patch <<< "$OLD"

case "$PART" in
  major) major=$((major + 1)); minor=0; patch=0 ;;
  minor) minor=$((minor + 1)); patch=0 ;;
  patch) patch=$((patch + 1)) ;;
esac

NEW="${major}.${minor}.${patch}"
printf '%s\n' "$NEW" > "$VERSION_FILE"

# Clean CMake cache so the new version is picked up
rm -rf build/CMakeFiles/llama-cli.dir/flags.make 2>/dev/null || true

echo "$OLD -> $NEW"
