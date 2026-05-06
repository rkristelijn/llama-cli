#!/usr/bin/env bash
# check-slop.sh — Detect common AI-generated code slop patterns.
# Checks git diff against main for signs of low-quality AI output.
# Signals: over-commenting, redundant error handling, dead patterns.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

DIFF=$(git diff main...HEAD -- 'src/*.cpp' 'src/*.h' 'src/**/*.cpp' 'src/**/*.h' 2>/dev/null || true)
if [[ -z "$DIFF" ]]; then
  echo "  [skip] no C++ changes vs main"
  exit 0
fi

WARNINGS=0

# 1. Obvious comment that restates the code (e.g., "// increment i" above "i++")
count=$(echo "$DIFF" | grep -c '^\+.*// \(set\|get\|return\|increment\|decrement\|initialize\|assign\) ' || true)
if [[ $count -gt 5 ]]; then
  echo "  ⚠ $count obvious/redundant comments (comments should explain WHY, not WHAT)"
  ((WARNINGS++))
fi

# 2. Empty catch blocks or swallowed exceptions
count=$(echo "$DIFF" | grep -c '^\+.*catch.*{[[:space:]]*}' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ $count empty catch blocks (swallowed errors)"
  ((WARNINGS++))
fi

# 3. Unnecessary nullptr checks right after construction
count=$(echo "$DIFF" | grep -c '^\+.*if.*!=.*nullptr.*//.*should never' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ $count defensive nullptr checks with 'should never' comments"
  ((WARNINGS++))
fi

# 4. TODO/FIXME/HACK added in new code (AI often leaves these)
count=$(echo "$DIFF" | grep -c '^\+.*// \(TODO\|FIXME\|HACK\|XXX\):' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ $count TODO/FIXME markers in new code (resolve before committing)"
  ((WARNINGS++))
fi

# 5. Excessive blank lines (3+ consecutive)
count=$(echo "$DIFF" | grep -c '^\+$' || true)
total_added=$(echo "$DIFF" | grep -c '^\+' || true)
if [[ $total_added -gt 0 ]]; then
  blank_ratio=$((count * 100 / total_added))
  if [[ $blank_ratio -gt 15 ]]; then
    echo "  ⚠ ${blank_ratio}% blank lines in additions (max 15%)"
    ((WARNINGS++))
  fi
fi

# 6. Repeated boilerplate patterns (same line appearing 4+ times)
dupes=$(echo "$DIFF" | grep '^\+' | sort | uniq -c | sort -rn | awk '$1 >= 4 && $2 !~ /^[\+\-\s{}]$/ {print}' | head -3)
if [[ -n "$dupes" ]]; then
  echo "  ⚠ repeated boilerplate patterns:"
  echo "$dupes" | sed 's/^/    /'
  ((WARNINGS++))
fi

if [[ $WARNINGS -eq 0 ]]; then
  echo "  [done] slop-check (clean)"
else
  echo ""
  echo "  $WARNINGS slop signal(s) detected — review for AI-generated bloat"
  # Non-blocking: exit 0 (advisory only)
fi
