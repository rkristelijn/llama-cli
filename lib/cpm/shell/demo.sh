#!/usr/bin/env bash
# demo.sh — Showcase cpm UI features
# Usage: cpm demo [spinners|ui|all]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ui.sh"

demo_spinners() {
  print_header "Spinners (CPM_SPINNER=name)"
  for style in dots dots2 dots3 dots9 dots10 bounce dots13 pipe arc arrow line star star2 sand; do
    CPM_SPINNER="$style" spinner_start "$style"
    sleep 0.8
    spinner_stop
    printf "  %-10s" "$style"
    # Show frames
    CPM_SPINNER="$style" spinner_start ""
    sleep 0.01
    spinner_stop
    echo ""
  done
  echo ""
  echo "  Set: CPM_SPINNER=pipe or cpm.toml [ui] spinner = \"pipe\""
}

demo_ui() {
  print_header "Status indicators"
  print_step 1 "passing-check" success "230ms"
  print_step 2 "slow-check" success "6.2s [slow]"
  print_step 3 "very-slow" success "14s [SLOW]"
  print_step "" "skipped-check" skip "no matching changes"
  print_step 4 "failing-check" error
  echo ""
  print_error "something went wrong at src/main.cpp:42"
  print_warning "complexity=15 (max 10)"
  echo ""
  print_summary "5 checks (3 passed, 1 skipped, 1 failed)"

  echo ""
  print_header "Progress bar"
  for i in $(seq 1 20); do
    progress_bar "$i" 20 "processing files"
    sleep 0.05
  done
}

case "${1:-all}" in
  spinners) demo_spinners ;;
  ui)       demo_ui ;;
  *)        demo_spinners; echo ""; demo_ui ;;
esac
