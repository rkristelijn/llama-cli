#!/usr/bin/env bash
# format-emojis.sh — Replace emojis with Unicode equivalents in markdown
# Usage: format-emojis.sh [file ...]
# Converts emoji characters to Unicode escape sequences for consistency
#
# Description:
#   Standardizes emoji usage in markdown files by replacing emoji characters
#   with mapped Unicode equivalents. Supports batch processing of all markdown
#   files in docs/ or individual file arguments.
#
# Examples:
#   format-emojis.sh                          # Process all docs/**/*.md
#   format-emojis.sh docs/adr/adr-104.md      # Process single file
#
# Exit codes:
#   0 - Success
#   1 - Error during processing

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
# Emoji to Unicode mapping (common in ADRs)
declare -A EMOJI_MAP=(
  ["✅"]="✓"
  ["❌"]="✗"
  ["⚠️"]="⚠"
  ["ℹ️"]="ℹ"
  ["📝"]="📝"
  ["🔧"]="🔧"
  ["📊"]="📊"
  ["🚀"]="🚀"
  ["💡"]="💡"
  ["⏱️"]="⏱"
  ["🎯"]="🎯"
  ["📈"]="📈"
  ["🔍"]="🔍"
  ["✨"]="✨"
  ["🎉"]="🎉"
  ["⚡"]="⚡"
  ["🛠️"]="🛠"
  ["📚"]="📚"
  ["🔐"]="🔐"
  ["🌟"]="🌟"
)

format_file() {
  local file="$1"
  local temp_file="${file}.tmp"

  cp "$file" "$temp_file"

  for emoji in "${!EMOJI_MAP[@]}"; do
    unicode="${EMOJI_MAP[$emoji]}"
    sed -i '' "s/${emoji}/${unicode}/g" "$temp_file"
  done

  if ! diff -q "$file" "$temp_file" >/dev/null 2>&1; then
    mv "$temp_file" "$file"
    print_step "" "$file" success
  else
    rm "$temp_file"
  fi
}

if [ $# -eq 0 ]; then
  print_header "formatting emojis in markdown files..."
  find docs -name '*.md' -type f | while read -r file; do
    format_file "$file"
  done
else
  for file in "$@"; do
    format_file "$file"
  done
fi
