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
  ((WARNINGS++)) || true || true
fi

# 2. Empty catch blocks
count=$(echo "$DIFF" | grep -c '^\+.*catch.*{[[:space:]]*}' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Empty catch blocks: $count (errors silently swallowed)"
  echo "    → Log or handle the error, don't ignore it"
  ((WARNINGS++)) || true
fi

# 3. Defensive nullptr checks with "should never" comments
count=$(echo "$DIFF" | grep -c '^\+.*if.*!=.*nullptr.*//.*should never' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Paranoid nullptr checks: $count with 'should never happen' comments"
  echo "    → If it should never happen, use an assert instead"
  ((WARNINGS++)) || true
fi

# 4. Too many TODO/FIXME in new code
count=$(echo "$DIFF" | grep -c '^\+.*// \(TODO\|FIXME\|HACK\|XXX\):' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ Unresolved markers: $count TODO/FIXME/HACK in new code"
  echo "    → Resolve before committing, or move to TODO.md"
  ((WARNINGS++)) || true
fi

# 5. Excessive blank lines
count=$(echo "$DIFF" | grep -c '^\+$' || true)
total_added=$(echo "$DIFF" | grep -c '^\+' || true)
if [[ $total_added -gt 0 ]]; then
  blank_ratio=$((count * 100 / total_added))
  if [[ $blank_ratio -gt 15 ]]; then
    echo "  ⚠ Blank line bloat: ${blank_ratio}% of additions are empty lines (max 15%)"
    echo "    → Remove unnecessary spacing"
    ((WARNINGS++)) || true
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
  ((WARNINGS++)) || true
fi

# 7. Trivial docstrings on short functions (/// comment above a 1-3 line function)
count=$(echo "$DIFF" | grep -c '^\+.*/// \(Get\|Set\|Return\|Check\|Create\|Initialize\) ' || true)
if [[ $count -gt 8 ]]; then
  echo "  ⚠ Trivial docstrings: $count 'Get/Set/Return/Check' style comments"
  echo "    → Only document non-obvious behavior, not the function name"
  ((WARNINGS++)) || true
fi

# 8. Over-commenting: more comment lines than code lines in additions
comment_lines=$(echo "$DIFF" | grep -c '^\+[[:space:]]*//' || true)
code_lines=$(echo "$DIFF" | grep -c '^\+' | head -1 || true)
if [[ $code_lines -gt 50 ]]; then
  comment_pct=$((comment_lines * 100 / code_lines))
  if [[ $comment_pct -gt 40 ]]; then
    echo "  ⚠ Over-commenting: ${comment_pct}% of new lines are comments (max 40%)"
    echo "    → Comments should be sparse and meaningful, not a running narration"
    ((WARNINGS++)) || true
  fi
fi

# 9. Single-implementation abstractions (abstract class with only 1 subclass in diff)
abstract_count=$(echo "$DIFF" | grep -c '^\+.*virtual.*= 0' || true)
impl_count=$(echo "$DIFF" | grep -c '^\+.*override' || true)
if [[ $abstract_count -gt 3 && $impl_count -le $abstract_count ]]; then
  echo "  ⚠ Abstraction inflation: $abstract_count virtual methods but only $impl_count overrides"
  echo "    → Don't create interfaces until you have 2+ implementations (YAGNI)"
  ((WARNINGS++)) || true
fi

# 10. Cargo cult: unnecessary std::move on trivial types or string literals
count=$(echo "$DIFF" | grep -c '^\+.*std::move("' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Cargo cult: $count std::move on string literals (does nothing)"
  echo "    → std::move on literals/temporaries is pointless"
  ((WARNINGS++)) || true
fi

# 11. Semantic duplication: same function body appearing in multiple new functions
func_bodies=$(echo "$DIFF" | grep '^\+' | grep -E '^\+\s+(return|if \(|for \(|while \()' | sort | uniq -d | wc -l)
if [[ $func_bodies -gt 10 ]]; then
  echo "  ⚠ Semantic duplication: $func_bodies repeated logic patterns"
  echo "    → Extract shared logic into a helper function"
  ((WARNINGS++)) || true
fi

# 12. AI tell-words in comments (delve, comprehensive, leverage, utilize, facilitate)
count=$(echo "$DIFF" | grep -ci '^\+.*//.*\(delve\|comprehensive\|leverage\|utilize\|facilitate\|streamline\|robust\)' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ AI vocabulary: $count comments use 'delve/comprehensive/leverage/utilize' style"
  echo "    → Use plain language: 'use' not 'utilize', 'strong' not 'robust'"
  ((WARNINGS++)) || true
fi

# --- Documentation slop (markdown files) ---
MD_DIFF=$(git diff main...HEAD -- '*.md' 2>/dev/null || true)
if [[ -n "$MD_DIFF" ]]; then
  # 13. AI tell-words in docs
  md_tells=$(echo "$MD_DIFF" | grep -ci '^\+.*\(delve\|comprehensive\|leverage\|utilize\|facilitate\|streamline\|in today.s rapidly\|it.s important to note\)' || true)
  if [[ $md_tells -gt 5 ]]; then
    echo "  ⚠ AI vocabulary in docs: $md_tells uses of 'delve/comprehensive/leverage/utilize'"
    echo "    → Write like a human: direct, specific, no filler"
    ((WARNINGS++)) || true
  fi

  # 14. Filler paragraphs
  filler=$(echo "$MD_DIFF" | grep -ci '^\+.*\(in summary\|as mentioned\|it is worth noting\|needless to say\|at the end of the day\)' || true)
  if [[ $filler -gt 3 ]]; then
    echo "  ⚠ Filler phrases in docs: $filler empty-calorie sentences"
    echo "    → Delete sentences that don't add new information"
    ((WARNINGS++)) || true
  fi

  # 15. Over-hedging
  hedging=$(echo "$MD_DIFF" | grep -ci '^\+.*\(it.s important to\|it should be noted\|one might argue\|arguably\|to some extent\|in some cases\)' || true)
  if [[ $hedging -gt 3 ]]; then
    echo "  ⚠ Over-hedging in docs: $hedging wishy-washy qualifiers"
    echo "    → Take a stance. Be direct."
    ((WARNINGS++)) || true
  fi
fi

# Summary
echo ""
if [[ $WARNINGS -eq 0 ]]; then
  echo "  ✓ No slop detected — code looks hand-crafted"
else
  echo "  $WARNINGS pattern(s) found — review for AI-generated bloat"
fi
echo "  [done] slop"
