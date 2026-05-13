#!/usr/bin/env bash
#
# check-file-size.sh — Enforce maximum file sizes (net code lines) and max files per directory.
#
# Net code = total lines minus comments and blank lines.
# Comments don't count — they're documentation, not complexity.
#
# Limits (configurable via env):
#   CPP source: 600 net code lines
#   Headers:    300 net code lines
#   Tests:      700 net code lines
#   Scripts:    200 net code lines
#   Max files per src/ subdirectory: 15
#
# @see docs/adr/adr-061-file-size-limits.md
# @see docs/adr/adr-121-cpm-quality-layer.md

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

# Load config from cpm.toml (or fallback defaults)
if [[ -f "${CPM_CONFIG:-lib/cpm/shell/config.sh}" ]]; then
  source "${CPM_CONFIG:-lib/cpm/shell/config.sh}"
fi

# Limits from cpm.toml (via config.sh) or env override
MAX_SOURCE="${CPM_LIMIT_SOURCE_LINES:-600}"
MAX_HEADER="${CPM_LIMIT_HEADER_LINES:-300}"
MAX_TEST="${CPM_LIMIT_TEST_LINES:-700}"
MAX_SCRIPT="${CPM_LIMIT_SCRIPT_LINES:-200}"
MAX_FILES_PER_DIR="${CPM_LIMIT_FILES_PER_DIR:-20}"

# Known violations — exempt until split (ADR-061)
EXEMPT=(
  "src/repl/repl_commands.cpp"
  "src/repl/repl_test.cpp"
  "src/annotation/annotation.cpp"
  "src/config/config.cpp"
)

# In-file annotation: FILE-SIZE-EXEMPT: <reason>
# Recognized reasons and their multiplier:
#   dispatch-table  → 2x limit (switch/case, command maps)
#   test-suite      → uses MAX_TEST already
#   generated       → skip entirely
#   data            → skip entirely
get_file_multiplier() {
  local file="$1"
  local reason
  reason=$(grep -o 'FILE-SIZE-EXEMPT: *[a-z-]*' "$file" 2>/dev/null | head -1 | sed 's/FILE-SIZE-EXEMPT: *//')
  case "$reason" in
  dispatch-table) echo 2 ;;
  generated | data) echo 0 ;; # 0 = skip
  *) echo 1 ;;
  esac
}

# Count net code lines (strip comments and blanks)
# Works for C++ (// and /* */ style) and shell (# style)
count_code_lines() {
  local file="$1"
  case "$file" in
  *.sh)
    grep -cvE '^\s*(#|$)' "$file" 2>/dev/null || echo 0
    ;;
  *)
    grep -cvE '^\s*(//|/\*|\*|\*/|\s*$)' "$file" 2>/dev/null || echo 0
    ;;
  esac
}

is_exempt() {
  local file="$1"
  for e in "${EXEMPT[@]}"; do
    [[ "$file" == "$e" ]] && return 0
  done
  return 1
}

check_file_sizes() {
  local failed=0

  # C++ files
  while IFS= read -r file; do
    local multiplier
    multiplier=$(get_file_multiplier "$file")

    # Skip files marked as generated/data
    [[ "$multiplier" == "0" ]] && continue

    local lines
    lines=$(count_code_lines "$file")

    # Determine limit based on file type
    local max
    if [[ "$file" == *_test.cpp || "$file" == *_it.cpp ]]; then
      max=$MAX_TEST
    elif [[ "$file" == *.h ]]; then
      max=$MAX_HEADER
    else
      max=$MAX_SOURCE
    fi

    # Apply multiplier (dispatch-table gets 2x)
    max=$((max * multiplier))

    if ((lines > max)); then
      if is_exempt "$file"; then
        print_warning "$file ($lines/$max net lines) — exempt, see ADR-061"
      else
        print_error "$file: $lines net code lines (max $max)"
        failed=1
      fi
    fi
  done < <(find src -name '*.cpp' -o -name '*.h')

  # Shell scripts
  while IFS= read -r file; do
    local multiplier
    multiplier=$(get_file_multiplier "$file")
    [[ "$multiplier" == "0" ]] && continue

    local lines
    lines=$(count_code_lines "$file")
    local max=$((MAX_SCRIPT * multiplier))
    if ((lines > max)); then
      print_error "$file: $lines net code lines (max $max)"
      failed=1
    fi
  done < <(find scripts -name '*.sh' 2>/dev/null)

  return $failed
}

check_files_per_dir() {
  local failed=0

  while IFS= read -r dir; do
    local count
    count=$(find "$dir" -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.h' \) | wc -l | tr -d ' ')
    if ((count > MAX_FILES_PER_DIR)); then
      print_error "$dir: $count files (max $MAX_FILES_PER_DIR) — split into submodules"
      failed=1
    fi
  done < <(find src -type d)

  return $failed
}

main() {
  print_header "checking file sizes (net code, ADR-061)..."
  local failed=0

  check_file_sizes || failed=1
  check_files_per_dir || failed=1

  if ((failed)); then
    print_step 1 "file-size" error
    exit 1
  fi
  print_step 1 "file-size" success
}

main "$@"
