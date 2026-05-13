#!/usr/bin/env bash
#
# check-smells.sh — Detect common engineering anti-patterns and foot-guns.
#
# Inspired by the "101 things that trigger developers" list.
# Some are serious (security, correctness), others are cultural (¬_¬)
#
# Usage:
#   bash scripts/lint/check-smells.sh
#   make smells
#
# @see frown.log

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
WARN=0
FAIL=0

warn() {
  printf "  ⚠ %s\n" "$1"
  WARN=$((WARN + 1))
  return 0
}
pass() {
  printf "  ✓ %s\n" "$1"
  return 0
}

main() {
  echo "==> engineering smell check (¬_¬)"
  echo ""

  # ── #58: "Temporary fix" that stays forever ──
  echo "── Temporary fixes ──"
  local temps
  temps=$(grep -rniE "temporary|temp fix|hack|workaround|FIXME" src/ \
    --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "check-smells\|template\|temperature" | wc -l || echo 0)
  if [[ "$temps" -gt 5 ]]; then
    warn "$temps temporary/hack/workaround markers — are these still needed?"
  else
    pass "Few temporary markers ($temps)"
  fi

  # ── #60: Friday deploys ──
  echo ""
  echo "── Friday deploys ──"
  local day
  day=$(date +%u) # 5 = Friday
  if [[ "$day" -eq 5 ]]; then
    warn "It's Friday — deploy with caution (ノಠ益ಠ)ノ彡┻━┻"
  else
    pass "Not Friday — deploy freely"
  fi

  # ── #82/#84/#85: Time-related foot-guns ──
  echo ""
  echo "── Time bombs ──"
  local time_issues
  time_issues=$(grep -rn "/ *365\|\\* *365\|86400" src/ --include="*.cpp" --include="*.h" 2>/dev/null |
    grep -v "check-smells" | wc -l || echo 0)
  if [[ "$time_issues" -gt 0 ]]; then
    warn "$time_issues uses of magic time constants (365/86400) — leap year safe?"
  else
    pass "No magic time constants"
  fi

  # Datetime without timezone (localtime without UTC)
  local tz_issues
  tz_issues=$(grep -rnE "localtime[^_]" src/ --include="*.cpp" --include="*.h" 2>/dev/null |
    grep -v "gmtime\|UTC\|_test\|_it\.\|check-smells\|timezone\|utc" | wc -l || echo 0)
  if [[ "$tz_issues" -gt 0 ]]; then
    warn "$tz_issues uses of localtime() without UTC — use gmtime + Z suffix"
  else
    pass "All timestamps use UTC (no raw localtime)"
  fi

  # ── #88: Floating point comparison ──
  echo ""
  echo "── Floating point ──"
  local fp_eq
  fp_eq=$(grep -rnE "==.*\b[0-9]+\.[0-9]+|[0-9]+\.[0-9]+.*==" src/ --include="*.cpp" 2>/dev/null |
    grep -v "check-smells\|npos\|string::" | wc -l || echo 0)
  if [[ "$fp_eq" -gt 0 ]]; then
    warn "$fp_eq direct float equality comparisons (use epsilon)"
  else
    pass "No direct float == comparisons"
  fi

  # ── #55: Debug output in production ──
  echo ""
  echo "── Debug leftovers ──"
  local debug
  debug=$(grep -rnE "std::cout.*debug|std::cerr.*debug|printf.*debug|std::cout.*TODO" src/ \
    --include="*.cpp" 2>/dev/null | grep -v "check-smells\|test_\|_test\|_it\." | wc -l || echo 0)
  if [[ "$debug" -gt 0 ]]; then
    warn "$debug debug print statements in production code"
  else
    pass "No debug prints in production code"
  fi

  # ── #66: Over-abstraction ──
  echo ""
  echo "── Over-engineering ──"
  local interfaces
  interfaces=$(find src -name "*.h" | xargs grep -l "virtual.*= 0" 2>/dev/null | wc -l || echo 0)
  local impl_files
  impl_files=$(find src -name "*.cpp" | wc -l || echo 0)
  local ratio=$((interfaces * 100 / (impl_files + 1)))
  if [[ "$ratio" -gt 50 ]]; then
    warn "Interface-to-implementation ratio: ${ratio}% — enterprise Java vibes?"
  else
    pass "Reasonable abstraction level (${interfaces} interfaces / ${impl_files} impl files)"
  fi

  # ── #62: Dependency count ──
  echo ""
  echo "── Dependencies ──"
  local deps
  deps=$(grep -c "FetchContent_Declare" CMakeLists.txt 2>/dev/null || echo 0)
  if [[ "$deps" -gt 10 ]]; then
    warn "$deps dependencies — left-pad territory?"
  else
    pass "Lean dependency count: $deps (not node_modules)"
  fi

  # ── #36/#37: Clean code dogma ──
  echo ""
  echo "── Complexity balance ──"
  local one_liners
  one_liners=$(find src -name "*.cpp" -exec awk '
    /^[a-zA-Z].*\(.*\).*\{/ { start=NR }
    /^\}/ { if (NR-start <= 2 && start > 0) count++ }
    END { print count+0 }
  ' {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')
  local total_funcs
  total_funcs=$(find src -name "*.cpp" -exec grep -c "^[a-zA-Z].*(.*).*{" {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')
  if [[ "$total_funcs" -gt 0 ]]; then
    local trivial_pct=$((one_liners * 100 / total_funcs))
    if [[ "$trivial_pct" -gt 60 ]]; then
      warn "${trivial_pct}% trivial functions — over-extracted? KISS > SOLID"
    else
      pass "Good extraction balance (${trivial_pct}% trivial functions)"
    fi
  fi

  # ── #101: Scope creep ──
  echo ""
  echo "── Scope ──"
  local loc
  loc=$(find src -name "*.cpp" -o -name "*.h" | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
  local adrs
  adrs=$(find docs/adr -name "*.md" 2>/dev/null | wc -l || echo 0)
  if [[ "$adrs" -gt "$((loc / 100))" ]]; then
    warn "More ADRs ($adrs) than code warrants ($loc LOC) — documenting more than building?"
  else
    pass "ADR-to-code ratio reasonable ($adrs ADRs / $loc LOC)"
  fi

  # ── Result ──
  echo ""
  echo "── Result ──"
  if [[ $FAIL -eq 0 && $WARN -eq 0 ]]; then
    echo "  ✓ No smells detected ╰(°▽°)╯"
  elif [[ $FAIL -eq 0 ]]; then
    echo "  ⚠ $WARN smell(s) — not blocking, just... (¬_¬)"
  else
    echo "  ✗ $FAIL serious smell(s) — fix before shipping"
    exit 1
  fi
  echo "  [done] smells"
}

main "$@"
