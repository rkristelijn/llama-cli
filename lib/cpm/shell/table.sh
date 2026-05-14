#!/usr/bin/env bash
# Table formatting helper — handles ANSI codes in column width calculation

# Print table row with proper alignment (accounts for ANSI escape codes)
print_table_row() {
  local -a cols=("$@")
  local term_width
  term_width=$(stty size 2>/dev/null | cut -d' ' -f2)
  [[ -z "$term_width" ]] && term_width=80

  # Compact widths for narrow terminals
  local -a widths
  if [[ $term_width -lt 80 ]]; then
    widths=(20 8 6 6 4 4 4 4)
  else
    widths=(28 10 8 8 6 6 6 6)
  fi

  for i in "${!cols[@]}"; do
    local col="${cols[$i]}"
    local width="${widths[$i]}"

    # Calculate visible length (strip ANSI codes)
    local visible; visible=$(echo -e "$col" | sed 's/\x1b\[[0-9;]*m//g')
    local visible_len=${#visible}

    # Calculate padding needed
    local padding=$((width - visible_len))
    [[ $padding -lt 0 ]] && padding=0

    # Print column with padding
    printf "%b%*s" "$col" "$padding" ""
  done
  printf "\n"
}

# Print table separator
print_table_separator() {
  local term_width
  term_width=$(stty size 2>/dev/null | cut -d' ' -f2)
  [[ -z "$term_width" ]] && term_width=80
  local total_width=$((term_width < 80 ? 56 : 78))
  printf "%.s─" $(seq 1 "$total_width")
  printf "\n"
}

# Print table header
print_table_header() {
  print_table_row "Check" "Status" "Autofix" "Commit" "Push" "Make" "CI" "npm"
  print_table_separator
}
