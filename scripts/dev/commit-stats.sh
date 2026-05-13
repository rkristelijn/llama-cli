#!/usr/bin/env bash
#
# commit-stats.sh — Analyze commit distribution by type, scope, and category.
#
# Usage:
#   bash scripts/dev/commit-stats.sh              # since last 6 months
#   bash scripts/dev/commit-stats.sh --since 2026-01-01
#   bash scripts/dev/commit-stats.sh --since 2026-01-01 --format csv
#
# Output: type breakdown, scope breakdown, fix subcategories, rework ratio.
# @see docs/adr/adr-099-right-first-time.md

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
# --- Defaults ---
SINCE="6 months ago"
FORMAT="table"

# --- Parse args ---
while [[ $# -gt 0 ]]; do
  case "$1" in
  --since)
    SINCE="$2"
    shift 2
    ;;
  --format)
    FORMAT="$2"
    shift 2
    ;;
  *)
    echo "Unknown arg: $1"
    exit 1
    ;;
  esac
done

# --- Gather commits ---
COMMITS=$(git log --oneline --since="$SINCE" 2>/dev/null)
TOTAL=$(echo "$COMMITS" | wc -l | tr -d ' ')

if [[ "$TOTAL" -eq 0 ]]; then
  echo "No commits found since $SINCE"
  exit 0
fi

# --- Type breakdown ---
count_type() { echo "$COMMITS" | grep -c "^[a-f0-9]* $1" 2>/dev/null || true; }

FIX=$(count_type "fix")
FEAT=$(count_type "feat")
DOCS=$(count_type "docs")
CHORE=$(count_type "chore")
REFACTOR=$(count_type "refactor")
TEST=$(count_type "test")
CI=$(count_type "ci")
PERF=$(count_type "perf")
BUILD=$(count_type "build")
STYLE=$(count_type "style")
# Ensure numeric (grep -c returns empty on some systems)
FIX=${FIX:-0}
FEAT=${FEAT:-0}
DOCS=${DOCS:-0}
CHORE=${CHORE:-0}
REFACTOR=${REFACTOR:-0}
TEST=${TEST:-0}
CI=${CI:-0}
PERF=${PERF:-0}
BUILD=${BUILD:-0}
STYLE=${STYLE:-0}
OTHER=$((TOTAL - FIX - FEAT - DOCS - CHORE - REFACTOR - TEST - CI - PERF - BUILD - STYLE))

# --- Fix subcategories ---
FIX_LINES=$(echo "$COMMITS" | grep "^[a-f0-9]* fix" || true)
FIX_CI=$(echo "$FIX_LINES" | grep -ciE "ci|pipeline|coverage|lint|format" || echo 0)
FIX_TEST=$(echo "$FIX_LINES" | grep -ciE "test|assert|scenario" || echo 0)
FIX_SECURITY=$(echo "$FIX_LINES" | grep -ciE "sonar|semgrep|security|sast" || echo 0)
FIX_BUGS=$((FIX - FIX_CI - FIX_TEST - FIX_SECURITY))

# --- Scope breakdown ---
SCOPES=$(echo "$COMMITS" | grep -oP '(?<=\()[^)]+(?=\))' | sort | uniq -c | sort -rn | head -15)

# --- Rework ratio ---
# Rework = fixes caused by tooling, not by product bugs
REWORK=$((FIX_CI + FIX_SECURITY))
if [[ "$FIX" -gt 0 ]]; then
  REWORK_PCT=$((REWORK * 100 / FIX))
else
  REWORK_PCT=0
fi

# Productive commits = feat + fix (actual bugs)
PRODUCTIVE=$((FEAT + FIX_BUGS))
if [[ "$PRODUCTIVE" -gt 0 ]]; then
  FIX_RATIO=$((FIX_BUGS * 100 / PRODUCTIVE))
else
  FIX_RATIO=0
fi

# --- Output ---
pct() { echo "$((${1} * 100 / TOTAL))"; }

if [[ "$FORMAT" == "csv" ]]; then
  echo "type,count,percent"
  echo "fix,$FIX,$(pct "$FIX")"
  echo "feat,$FEAT,$(pct "$FEAT")"
  echo "docs,$DOCS,$(pct "$DOCS")"
  echo "chore,$CHORE,$(pct "$CHORE")"
  echo "refactor,$REFACTOR,$(pct "$REFACTOR")"
  echo "test,$TEST,$(pct "$TEST")"
  echo "ci,$CI,$(pct "$CI")"
  echo "perf,$PERF,$(pct "$PERF")"
  echo "build,$BUILD,$(pct "$BUILD")"
  echo "style,$STYLE,$(pct "$STYLE")"
  echo "other,$OTHER,$(pct "$OTHER")"
else
  echo "=== Commit Distribution (since: $SINCE) ==="
  echo "  Total: $TOTAL"
  echo ""
  printf "  %-12s %5s %5s\n" "Type" "Count" "%"
  printf "  %-12s %5s %5s\n" "----" "-----" "---"
  printf "  %-12s %5d %4d%%\n" "fix" "$FIX" "$(pct "$FIX")"
  printf "  %-12s %5d %4d%%\n" "feat" "$FEAT" "$(pct "$FEAT")"
  printf "  %-12s %5d %4d%%\n" "docs" "$DOCS" "$(pct "$DOCS")"
  printf "  %-12s %5d %4d%%\n" "chore" "$CHORE" "$(pct "$CHORE")"
  printf "  %-12s %5d %4d%%\n" "refactor" "$REFACTOR" "$(pct "$REFACTOR")"
  printf "  %-12s %5d %4d%%\n" "test" "$TEST" "$(pct "$TEST")"
  printf "  %-12s %5d %4d%%\n" "ci" "$CI" "$(pct "$CI")"
  printf "  %-12s %5d %4d%%\n" "perf" "$PERF" "$(pct "$PERF")"
  printf "  %-12s %5d %4d%%\n" "other" "$OTHER" "$(pct "$OTHER")"
  echo ""
  echo "=== Fix Subcategories ==="
  FIX_DIV=$((FIX > 0 ? FIX : 1))
  printf "  %-20s %5d (%d%% of fixes)\n" "CI/lint/format" "$FIX_CI" "$((FIX_CI * 100 / FIX_DIV))"
  printf "  %-20s %5d (%d%% of fixes)\n" "Test fixes" "$FIX_TEST" "$((FIX_TEST * 100 / FIX_DIV))"
  printf "  %-20s %5d (%d%% of fixes)\n" "Security tooling" "$FIX_SECURITY" "$((FIX_SECURITY * 100 / FIX_DIV))"
  printf "  %-20s %5d (%d%% of fixes)\n" "Actual bugs" "$FIX_BUGS" "$((FIX_BUGS * 100 / FIX_DIV))"
  echo ""
  echo "=== Top Scopes ==="
  echo "$SCOPES" | while read -r line; do echo "  $line"; done
  echo ""
  echo "=== Health Metrics ==="
  echo "  Tooling rework:    $REWORK/$FIX fixes ($REWORK_PCT%) are fixing the tooling itself"
  echo "  Bug fix ratio:     $FIX_BUGS/$PRODUCTIVE productive commits ($FIX_RATIO%) are bug fixes"
  echo ""
  echo "  Target: tooling rework <20%, bug fix ratio <40%"
fi
