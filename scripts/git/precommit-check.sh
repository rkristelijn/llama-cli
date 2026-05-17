#!/usr/bin/env bash
# precommit-check.sh — Auto-fix formatting + secret scan (smart: skips unchanged file types).
# Uses cpm runner for timing, logging, and consistent TUI output.
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true

# Detect which file types are staged
STAGED=$(git diff --cached --name-only --diff-filter=ACM 2>/dev/null || true)
HAS_CPP=false HAS_MD=false HAS_YAML=false HAS_SH=false HAS_IMG=false

while IFS= read -r f; do
  case "$f" in
  *.cpp | *.h) HAS_CPP=true ;;
  *.md) HAS_MD=true ;;
  *.yml | *.yaml) HAS_YAML=true ;;
  *.sh | Makefile) HAS_SH=true ;;
  *.png | *.bmp) HAS_IMG=true ;;
  esac
done <<<"$STAGED"

STEP=0 FAILED=0

run_step() {
  local name="$1"
  shift
  ((STEP++)) || true
  timer_start "$name"
  local output
  output=$("$@" 2>&1) && timer_stop "$name" success && return
  if echo "$output" | grep -qi "not installed\|skip\|not found"; then
    timer_stop "$name" skip
  else
    timer_stop "$name" error
    FAILED=$((FAILED + 1))
  fi
}

print_header "pre-commit"

# Formatting (autofix)
$HAS_CPP && run_step "format-code" bash lib/cpm/checks/cpp/format-code.sh
$HAS_YAML && run_step "format-yaml" bash lib/cpm/checks/universal/format-yaml.sh
$HAS_YAML && run_step "check-ci" bash scripts/lint/check-ci-yaml.sh
$HAS_MD && run_step "format-md" bash lib/cpm/checks/universal/format-md.sh
$HAS_SH && run_step "format-scripts" bash lib/cpm/checks/universal/format-scripts.sh

# Security
$HAS_IMG && run_step "sast-stegano" bash lib/cpm/checks/universal/steg-check.sh
run_step "sast-iac" bash lib/cpm/checks/universal/checkov-scan.sh
run_step "sast-secret" bash lib/cpm/checks/universal/sast-secret.sh
[[ -f .config/.pii ]] && run_step "check-pii" bash lib/cpm/checks/universal/check-pii.sh
$HAS_CPP && run_step "slop" bash lib/cpm/checks/universal/check-slop.sh

# Summary
echo ""
if ((FAILED == 0)); then
  print_summary "$STEP checks passed"
else
  print_error "$FAILED/$STEP checks failed"
  exit 1
fi
