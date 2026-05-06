#!/usr/bin/env bash
# check-slop.sh — Detect common AI-generated code slop patterns.
# Scans git diff against main for signs of low-quality AI output.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

echo "==> checking for AI slop patterns..."

DIFF=$(git diff main...HEAD -- 'src/*.cpp' 'src/*.h' 'src/**/*.cpp' 'src/**/*.h' 2>/dev/null || true)
if [[ -z "$DIFF" ]]; then
  echo "  [skip] no C++ changes vs main"
  exit 0
fi

WARNINGS=0

# 1. Obvious comments that restate the code
count=$(echo "$DIFF" | grep -c '^\+.*// \(set\|get\|return\|increment\|decrement\|initialize\|assign\) ' || true)
if [[ $count -gt 5 ]]; then
  echo "  ⚠ Redundant comments: $count lines restate what the code does"
  echo "    → Comments should explain WHY, not WHAT"
  ((WARNINGS++))
fi

# 2. Empty catch blocks
count=$(echo "$DIFF" | grep -c '^\+.*catch.*{[[:space:]]*}' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Empty catch blocks: $count (errors silently swallowed)"
  echo "    → Log or handle the error, don't ignore it"
  ((WARNINGS++))
fi

# 3. Defensive nullptr checks with "should never" comments
count=$(echo "$DIFF" | grep -c '^\+.*if.*!=.*nullptr.*//.*should never' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Paranoid nullptr checks: $count with 'should never happen' comments"
  echo "    → If it should never happen, use an assert instead"
  ((WARNINGS++))
fi

# 4. Too many TODO/FIXME in new code
count=$(echo "$DIFF" | grep -c '^\+.*// \(TODO\|FIXME\|HACK\|XXX\):' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ Unresolved markers: $count TODO/FIXME/HACK in new code"
  echo "    → Resolve before committing, or move to TODO.md"
  ((WARNINGS++))
fi

# 5. Excessive blank lines
count=$(echo "$DIFF" | grep -c '^\+$' || true)
total_added=$(echo "$DIFF" | grep -c '^\+' || true)
if [[ $total_added -gt 0 ]]; then
  blank_ratio=$((count * 100 / total_added))
  if [[ $blank_ratio -gt 15 ]]; then
    echo "  ⚠ Blank line bloat: ${blank_ratio}% of additions are empty lines (max 15%)"
    echo "    → Remove unnecessary spacing"
    ((WARNINGS++))
  fi
fi

# 6. Repeated boilerplate (same line 4+ times)
dupes=$(echo "$DIFF" | grep '^\+' | sed 's/^\+//' | grep -v '^$' | grep -v '^[[:space:]]*[{}]$' | sort | uniq -c | sort -rn | awk '$1 >= 4 && length($0) > 20 {print}' | head -3)
if [[ -n "$dupes" ]]; then
  echo "  ⚠ Repeated boilerplate (same line 4+ times):"
  echo "$dupes" | while read -r line; do
    echo "    $line"
  done
  echo "    → Extract to a function or loop"
  ((WARNINGS++))
fi

# Summary
echo ""
if [[ $WARNINGS -eq 0 ]]; then
  echo "  ✓ No slop detected — code looks hand-crafted"
else
  echo "  $WARNINGS pattern(s) found — review for AI-generated bloat"
fi
echo "  [done] slop"
