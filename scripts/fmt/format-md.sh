#!/usr/bin/env bash
# format-md.sh — Auto-format Markdown files using rumdl.
# Also auto-detects missing language tags on fenced code blocks (MD040).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Auto-detect language for bare ``` blocks based on content heuristics.
# Fixes MD040 violations in-place.
fix_code_block_languages() {
  local file="$1"
  local tmp="${file}.tmp"
  local in_block=false
  local block_start=0
  local first_line=""

  # Process line by line — when we find a bare ```, peek at next line to guess language
  awk '
  /^```$/ && !in_block {
    in_block = 1
    getline next_line
    lang = detect_lang(next_line)
    print "```" lang
    print next_line
    next
  }
  /^```/ && in_block {
    in_block = 0
    print
    next
  }
  /^```[a-zA-Z]/ && !in_block {
    in_block = 1
    print
    next
  }
  { print }

  function detect_lang(line) {
    # Shell indicators
    if (line ~ /^#!/) return "bash"
    if (line ~ /^\$ / || line ~ /^# In/) return "bash"
    if (line ~ /^(export |source |make |LLAMA_|OLLAMA_)/) return "bash"
    if (line ~ /^curl /) return "bash"

    # JSON
    if (line ~ /^\{/ || line ~ /^\[/) return "json"

    # C/C++
    if (line ~ /^#include/ || line ~ /^(struct|class|void|int|auto) /) return "cpp"
    if (line ~ /^(const |std::)/) return "cpp"

    # YAML
    if (line ~ /^- name:/ || line ~ /^[a-z_]+:/) return "yaml"

    # Python
    if (line ~ /^(def |import |from |class )/) return "python"

    # Diagrams/trees
    if (line ~ /[└├│─]/ || line ~ /^  +[└├]/) return "text"

    # SQL
    if (line ~ /^(SELECT|INSERT|CREATE|ALTER) /) return "sql"

    # Makefile
    if (line ~ /^[a-z_-]+:.*##/) return "makefile"

    # Default: text
    return "text"
  }
  ' "$file" > "$tmp"

  # Only replace if something changed
  if ! diff -q "$file" "$tmp" > /dev/null 2>&1; then
    mv "$tmp" "$file"
  else
    rm -f "$tmp"
  fi
}

if ! command -v rumdl >/dev/null; then
  echo "  [skip] rumdl not installed"
  exit 0
fi

echo "==> formatting markdown..."

# Phase 1: fix missing code block languages (MD040)
for f in $(find . -name '*.md' -not -path './node_modules/*' -not -path './build*' -not -path './.tmp/*'); do
  if grep -q '^```$' "$f" 2>/dev/null; then
    fix_code_block_languages "$f"
  fi
done

# Phase 2: run rumdl formatter
rumdl fmt .
echo "  [done] format-md"
