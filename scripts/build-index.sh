#!/bin/sh
# build-index.sh — Generate INDEX.md from all project files
# Reads frontmatter summary if present, falls back to first line.
# Output: INDEX.md in repo root

OUTPUT="INDEX.md"

echo "# Index" > "$OUTPUT"
echo "" >> "$OUTPUT"
echo "Auto-generated overview of all files in this repo." >> "$OUTPUT"
echo "" >> "$OUTPUT"

# Scan docs and code, skip build/ and node_modules
find docs src include test scripts -type f \( -name "*.md" -o -name "*.cpp" -o -name "*.h" -o -name "*.sh" \) 2>/dev/null | sort | while read -r file; do
  # Try to extract frontmatter summary
  summary=$(sed -n '/^---$/,/^---$/p' "$file" | grep '^summary:' | head -1 | sed 's/^summary: *//')

  # Fallback: first non-empty, non-frontmatter line
  if [ -z "$summary" ]; then
    summary=$(grep -m1 -v '^---$\|^$\|^#!' "$file" | head -1 | sed 's/^[# ]*//' | cut -c1-120)
  fi

  echo "- [\`$file\`]($file) — $summary" >> "$OUTPUT"
done

# Count files indexed
count=$(grep -c '^\- ' "$OUTPUT")
echo "" >> "$OUTPUT"
echo "_${count} files indexed._" >> "$OUTPUT"

echo "INDEX.md generated ($count files)"
