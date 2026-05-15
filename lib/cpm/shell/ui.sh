#!/usr/bin/env bash
# Shared UI library — single source of truth for terminal output.
#
# UI modes (auto-detected or set via CPM_UI_MODE):
#   auto      — detect from environment (default)
#   spinner   — animated spinner for unknown-duration tasks
#   progress  — progress bar for known-count tasks
#   checklist — sequential step list with ✓/✗
#   compact   — one-line per check (CI/pipe friendly)
#   silent    — only errors
#
# @see docs/adr/adr-121-cpm-quality-layer.md

# Auto-detect UI mode
_cpm_detect_mode() {
  if [[ -n "${CPM_UI_MODE:-}" ]]; then
    echo "$CPM_UI_MODE"
  elif [[ "${CI:-}" == "true" ]] || [[ ! -t 1 ]]; then
    echo "compact"
  elif [[ "${QUIET:-}" == "1" ]]; then
    echo "silent"
  else
    echo "checklist"
  fi
}

CPM_UI_MODE="${CPM_UI_MODE:-$(_cpm_detect_mode)}"

# Respect NO_COLOR standard for accessibility
if [[ -z "${NO_COLOR:-}" ]]; then
  RED='\033[0;91m'
  GREEN='\033[0;92m'
  YELLOW='\033[0;93m'
  GRAY='\033[0;90m'
  BOLD='\033[1m'
  RESET='\033[0m'
else
  RED='' GREEN='' YELLOW='' GRAY='' BOLD='' RESET=''
fi

CHECK="✓"
CROSS="✗"

# Step output for the pre-commit runner loop
print_step() {
  local num="$1" name="$2" status="$3" extra="${4:-}"

  # Terminal width — portable (no stty, works in CI/pipes/SSH)
  local term_width="${COLUMNS:-80}"

  local name_width=$((term_width < 80 ? 18 : 22))

  local prefix="  "
  [[ -n "$num" ]] && prefix="  [${num}] "
  printf "%s%-${name_width}s " "$prefix" "$name"

  case "$status" in
    success)
      echo -e "${GREEN}${CHECK}${RESET} ${GRAY}${extra}${RESET}"
      ;;
    error)
      echo -e "${RED}${CROSS}${RESET}"
      ;;
    skip)
      # Truncate if NOWRAP is set
      if [[ -n "${NOWRAP:-}" ]]; then
        # Calculate available space for message text
        local prefix_len=$((${#prefix} + name_width + 1 + 1 + 1))  # prefix + name + space + ⊘ + space
        local available=$((term_width - prefix_len))

        if [[ ${#extra} -gt $available ]]; then
          local truncated="${extra:0:$((available - 4))}..."
          echo -e "${GRAY}⊘ ${truncated}${RESET}"
        else
          echo -e "${GRAY}⊘ ${extra}${RESET}"
        fi
      else
        echo -e "${GRAY}⊘ ${extra}${RESET}"
      fi
      ;;
  esac
}

print_error()   { echo -e "${RED}ERROR:${RESET} $1"; }
print_warning() { echo -e "${YELLOW}WARNING:${RESET} $1"; }
print_summary() { echo ""; echo -e "${GREEN}All checks passed${RESET} in ${GRAY}$1${RESET}"; }
print_header()  { echo ""; echo -e "${GREEN}$1${RESET}"; echo ""; }

# Spinner — for unknown-duration tasks
# Usage: spinner_start "waiting..."; do_work; spinner_stop
_spinner_pid=""
spinner_start() {
  local msg="${1:-working...}"
  [[ "$CPM_UI_MODE" == "silent" ]] && return
  [[ "$CPM_UI_MODE" == "compact" ]] && return

  (
    local frames=('⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠇' '⠏')
    local i=0
    while true; do
      printf "\r  ${GRAY}${frames[$i]} %s${RESET}" "$msg" >&2
      i=$(( (i + 1) % ${#frames[@]} ))
      sleep 0.1
    done
  ) &
  _spinner_pid=$!
}

spinner_stop() {
  if [[ -n "$_spinner_pid" ]]; then
    kill "$_spinner_pid" 2>/dev/null
    wait "$_spinner_pid" 2>/dev/null || true
    printf "\r\033[K" >&2  # clear line
    _spinner_pid=""
  fi
}

# Progress bar — for known-count tasks
# Usage: progress_bar 23 51 "linting files"
progress_bar() {
  local current="$1" total="$2" label="${3:-}"
  [[ "$CPM_UI_MODE" == "silent" ]] && return
  [[ "$CPM_UI_MODE" == "compact" ]] && return

  local pct=$(( (current * 100) / total ))
  local filled=$(( pct / 5 ))  # 20 chars wide
  local empty=$(( 20 - filled ))
  local bar
  bar=$(printf '%0.s█' $(seq 1 $filled 2>/dev/null) 2>/dev/null || true)
  bar+=$(printf '%0.s░' $(seq 1 $empty 2>/dev/null) 2>/dev/null || true)

  printf "\r  [%s] %3d%% (%d/%d) %s" "$bar" "$pct" "$current" "$total" "$label" >&2

  # Newline when done
  ((current == total)) && echo "" >&2
}

# Auto-source timer if available (timing + trend analysis)
_cpm_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$_cpm_dir/timer.sh" ]]; then
  source "$_cpm_dir/timer.sh"
fi
