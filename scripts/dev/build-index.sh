#!/usr/bin/env bash
#
# build-index.sh — Generate INDEX.md from all project files
# Reads frontmatter summary if present, falls back to first line.
# Output: INDEX.md in repo root

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
OUTPUT="INDEX.md"

echo "# Index" >"$OUTPUT"
echo "" >>"$OUTPUT"
echo "Auto-generated overview of all files in this repo." >>"$OUTPUT"
echo "" >>"$OUTPUT"

# Scan docs and code, skip build/ and node_modules
dirs=""
for d in docs src include test scripts; do [ -d "$d" ] && dirs="$dirs $d"; done
find $dirs -type f \( -name "*.md" -o -name "*.cpp" -o -name "*.h" -o -name "*.sh" \) | sort | while read -r file; do
  # Try to extract frontmatter summary
  summary=$(sed -n '/^---$/,/^---$/p' "$file" | grep '^summary:' | head -1 | sed 's/^summary: *//' || true)

  # Fallback: first non-empty, non-frontmatter line
  if [ -z "$summary" ]; then
    summary=$(grep -m1 -v '^---$\|^$\|^#!' "$file" | head -1 | sed 's/^[# ]*//' | cut -c1-120 || true)
  fi

  summary="$(echo "$summary" | sed 's/[[:space:]]*$//')"
  if [ -n "$summary" ]; then
    echo "- [\`$file\`]($file) — ${summary}" >>"$OUTPUT"
  else
    echo "- [\`$file\`]($file)" >>"$OUTPUT"
  fi
done

# Count files indexed
count=$(grep -c '^\- ' "$OUTPUT")
echo "" >>"$OUTPUT"
echo "_${count} files indexed._" >>"$OUTPUT"

echo "INDEX.md generated ($count files)"
git add ./INDEX.md
